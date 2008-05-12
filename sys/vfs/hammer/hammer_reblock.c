/*
 * Copyright (c) 2008 The DragonFly Project.  All rights reserved.
 * 
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * $DragonFly: src/sys/vfs/hammer/hammer_reblock.c,v 1.14 2008/05/12 21:17:18 dillon Exp $
 */
/*
 * HAMMER reblocker - This code frees up fragmented physical space
 *
 * HAMMER only keeps track of free space on a big-block basis.  A big-block
 * containing holes can only be freed by migrating the remaining data in
 * that big-block into a new big-block, then freeing the big-block.
 *
 * This function is called from an ioctl or via the hammer support thread.
 */

#include "hammer.h"

static int hammer_reblock_helper(struct hammer_ioc_reblock *reblock,
				 hammer_cursor_t cursor,
				 hammer_btree_elm_t elm);
static int hammer_reblock_data(struct hammer_ioc_reblock *reblock,
				hammer_cursor_t cursor, hammer_btree_elm_t elm);
static int hammer_reblock_node(struct hammer_ioc_reblock *reblock,
				hammer_cursor_t cursor, hammer_btree_elm_t elm);

int
hammer_ioc_reblock(hammer_transaction_t trans, hammer_inode_t ip,
	       struct hammer_ioc_reblock *reblock)
{
	struct hammer_cursor cursor;
	hammer_btree_elm_t elm;
	int error;

	if (reblock->beg_obj_id >= reblock->end_obj_id)
		return(EINVAL);
	if (reblock->free_level < 0)
		return(EINVAL);

	reblock->cur_obj_id = reblock->beg_obj_id;

retry:
	error = hammer_init_cursor(trans, &cursor, NULL, NULL);
	if (error) {
		hammer_done_cursor(&cursor);
		return(error);
	}
	cursor.key_beg.obj_id = reblock->cur_obj_id;
	cursor.key_beg.key = HAMMER_MIN_KEY;
	cursor.key_beg.create_tid = 1;
	cursor.key_beg.delete_tid = 0;
	cursor.key_beg.rec_type = HAMMER_MIN_RECTYPE;
	cursor.key_beg.obj_type = 0;

	cursor.key_end.obj_id = reblock->end_obj_id;
	cursor.key_end.key = HAMMER_MAX_KEY;
	cursor.key_end.create_tid = HAMMER_MAX_TID - 1;
	cursor.key_end.delete_tid = 0;
	cursor.key_end.rec_type = HAMMER_MAX_RECTYPE;
	cursor.key_end.obj_type = 0;

	cursor.flags |= HAMMER_CURSOR_END_INCLUSIVE;
	cursor.flags |= HAMMER_CURSOR_BACKEND;

	error = hammer_btree_first(&cursor);
	while (error == 0) {
		elm = &cursor.node->ondisk->elms[cursor.index];
		reblock->cur_obj_id = elm->base.obj_id;

		/*
		 * Acquiring the sync_lock prevents the operation from
		 * crossing a synchronization boundary.
		 */
		hammer_lock_ex(&trans->hmp->sync_lock);
		error = hammer_reblock_helper(reblock, &cursor, elm);
		hammer_unlock(&trans->hmp->sync_lock);
		if (error == 0) {
			cursor.flags |= HAMMER_CURSOR_ATEDISK;
			error = hammer_btree_iterate(&cursor);
		}

		/*
		 * Bad hack for now, don't blow out the kernel's buffer
		 * cache.  NOTE: We still hold locks on the cursor, we
		 * cannot call the flusher synchronously.
		 */
		if (trans->hmp->locked_dirty_count > hammer_limit_dirtybufs) {
			hammer_flusher_async(trans->hmp);
			tsleep(trans, 0, "hmrslo", hz / 10);
		}
		if (error == 0)
			error = hammer_signal_check(trans->hmp);
	}
	if (error == ENOENT)
		error = 0;
	hammer_done_cursor(&cursor);
	if (error == EDEADLK)
		goto retry;
	if (error == EINTR) {
		reblock->head.flags |= HAMMER_IOC_HEAD_INTR;
		error = 0;
	}
	return(error);
}

/*
 * Reblock the B-Tree (leaf) node, record, and/or data if necessary.
 *
 * XXX We have no visibility into internal B-Tree nodes at the moment,
 * only leaf nodes.
 */
static int
hammer_reblock_helper(struct hammer_ioc_reblock *reblock,
		      hammer_cursor_t cursor, hammer_btree_elm_t elm)
{
	hammer_off_t tmp_offset;
	int error;
	int zone;
	int bytes;
	int cur;

	if (elm->leaf.base.btype != HAMMER_BTREE_TYPE_RECORD)
		return(0);
	error = 0;

	/*
	 * Reblock data.  Note that data embedded in a record is reblocked
	 * by the record reblock code.
	 */
	tmp_offset = elm->leaf.data_offset;
	zone = HAMMER_ZONE_DECODE(tmp_offset);		/* can be 0 */
	if ((zone == HAMMER_ZONE_SMALL_DATA_INDEX ||
	     zone == HAMMER_ZONE_LARGE_DATA_INDEX) &&
	    error == 0 && (reblock->head.flags & HAMMER_IOC_DO_DATA)) {
		++reblock->data_count;
		reblock->data_byte_count += elm->leaf.data_len;
		bytes = hammer_blockmap_getfree(cursor->trans->hmp, tmp_offset,
						&cur, &error);
		if (error == 0 && cur == 0 && bytes >= reblock->free_level) {
			if (hammer_debug_general & 0x4000)
				kprintf("%6d ", bytes);
			error = hammer_cursor_upgrade(cursor);
			if (error == 0) {
				error = hammer_reblock_data(reblock,
							    cursor, elm);
			}
			if (error == 0) {
				++reblock->data_moves;
				reblock->data_byte_moves += elm->leaf.data_len;
			}
		}
	}

	/*
	 * Reblock a B-Tree node.  Adjust elm to point at the parent's
	 * leaf entry.
	 */
	tmp_offset = cursor->node->node_offset;
	zone = HAMMER_ZONE_DECODE(tmp_offset);
	if (zone == HAMMER_ZONE_BTREE_INDEX && cursor->index == 0 &&
	    error == 0 && (reblock->head.flags & HAMMER_IOC_DO_BTREE)) {
		++reblock->btree_count;
		bytes = hammer_blockmap_getfree(cursor->trans->hmp, tmp_offset,
						&cur, &error);
		if (error == 0 && cur == 0 && bytes >= reblock->free_level) {
			if (hammer_debug_general & 0x4000)
				kprintf("%6d ", bytes);
			error = hammer_cursor_upgrade(cursor);
			if (error == 0) {
				if (cursor->parent)
					elm = &cursor->parent->ondisk->elms[cursor->parent_index];
				else
					elm = NULL;
				error = hammer_reblock_node(reblock,
							    cursor, elm);
			}
			if (error == 0) {
				++reblock->btree_moves;
			}
		}
	}

	hammer_cursor_downgrade(cursor);
	return(error);
}

/*
 * Reblock a record's data.  Both the B-Tree element and record pointers
 * to the data must be adjusted.
 */
static int
hammer_reblock_data(struct hammer_ioc_reblock *reblock,
		    hammer_cursor_t cursor, hammer_btree_elm_t elm)
{
	struct hammer_buffer *data_buffer = NULL;
	hammer_off_t ndata_offset;
	int error;
	void *ndata;

	error = hammer_btree_extract(cursor, HAMMER_CURSOR_GET_DATA |
					     HAMMER_CURSOR_GET_LEAF);
	if (error)
		return (error);
	ndata = hammer_alloc_data(cursor->trans, elm->leaf.data_len,
				  &ndata_offset, &data_buffer, &error);
	if (error)
		goto done;

	/*
	 * Move the data
	 */
	hammer_modify_buffer(cursor->trans, data_buffer, NULL, 0);
	bcopy(cursor->data, ndata, elm->leaf.data_len);
	hammer_modify_buffer_done(data_buffer);

	hammer_blockmap_free(cursor->trans,
			     elm->leaf.data_offset, elm->leaf.data_len);

	hammer_modify_node(cursor->trans, cursor->node,
			   &elm->leaf.data_offset, sizeof(hammer_off_t));
	elm->leaf.data_offset = ndata_offset;
	hammer_modify_node_done(cursor->node);

done:
	if (data_buffer)
		hammer_rel_buffer(data_buffer, 0);
	return (error);
}

/*
 * Reblock a B-Tree (leaf) node.  The parent must be adjusted to point to
 * the new copy of the leaf node.  elm is a pointer to the parent element
 * pointing at cursor.node.
 *
 * XXX reblock internal nodes too.
 */
static int
hammer_reblock_node(struct hammer_ioc_reblock *reblock,
		    hammer_cursor_t cursor, hammer_btree_elm_t elm)
{
	hammer_node_t onode;
	hammer_node_t nnode;
	int error;

	onode = cursor->node;
	nnode = hammer_alloc_btree(cursor->trans, &error);
	hammer_lock_ex(&nnode->lock);

	hammer_modify_node_noundo(cursor->trans, nnode);

	if (nnode == NULL)
		return (error);

	/*
	 * Move the node
	 */
	bcopy(onode->ondisk, nnode->ondisk, sizeof(*nnode->ondisk));

	if (elm) {
		/*
		 * We are not the root of the B-Tree 
		 */
		hammer_modify_node(cursor->trans, cursor->parent,
				   &elm->internal.subtree_offset,
				   sizeof(elm->internal.subtree_offset));
		elm->internal.subtree_offset = nnode->node_offset;
		hammer_modify_node_done(cursor->parent);
	} else {
		/*
		 * We are the root of the B-Tree
		 */
                hammer_volume_t volume;
                        
                volume = hammer_get_root_volume(cursor->trans->hmp, &error);
                KKASSERT(error == 0);

                hammer_modify_volume_field(cursor->trans, volume,
					   vol0_btree_root);
                volume->ondisk->vol0_btree_root = nnode->node_offset;
                hammer_modify_volume_done(volume);
                hammer_rel_volume(volume, 0);
        }

	hammer_delete_node(cursor->trans, onode);

	if (hammer_debug_general & 0x4000) {
		kprintf("REBLOCK NODE %016llx -> %016llx\n",
			onode->node_offset, nnode->node_offset);
	}
	hammer_modify_node_done(nnode);

	cursor->node = nnode;
	hammer_rel_node(onode);

	return (error);
}

