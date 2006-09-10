/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $FreeBSD: src/sys/contrib/dev/fla/fla.c,v 1.16 1999/12/08 04:45:16 ken Exp $ 
 * $DragonFly: src/sys/contrib/dev/fla/Attic/fla.c,v 1.15 2006/09/10 01:26:33 dillon Exp $ 
 *
 */

#include "use_fla.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/devicestat.h>
#include <sys/module.h>
#include <machine/clock.h>
#include <machine/resource.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_param.h>
#include <sys/buf2.h>
#include <sys/thread2.h>

#include <sys/bus.h>
#include <bus/isa/isavar.h>

#ifdef SMP
#include <machine/smp.h>
#define LEAVE() rel_mplock();			
#define ENTER() get_mplock();			
#else
#define LEAVE()
#define ENTER()
#endif

#include <contrib/dev/fla/msysosak.h>

MALLOC_DEFINE(M_FLA, "fla driver", "fla driver storage");

static int fla_debug = 0;
SYSCTL_INT(_debug, OID_AUTO, fladebug, CTLFLAG_RW, &fla_debug, 0, "");

#define CDEV_MAJOR	102
#define BDEV_MAJOR	28

static d_strategy_t flastrategy;
static d_open_t flaopen;
static d_close_t flaclose;
static d_ioctl_t flaioctl;

static struct dev_ops fla_ops = {
	{ "fla", CDEV_MAJOR, D_DISK | D_CANFREE },
        .d_open =	flaopen,
        .d_close =	flaclose,
        .d_read =	physread,
        .d_write =	physwrite,
        .d_ioctl =	flaioctl,
        .d_strategy =	flastrategy,
};

void *
doc2k_malloc(int bytes) 
{
	return kmalloc(bytes, M_FLA, M_WAITOK);
}

void
doc2k_free(void *ptr)
{
	kfree(ptr, M_FLA);
}

void
doc2k_delay(unsigned msec)
{
	DELAY(1000 * msec);
}

void
doc2k_memcpy(void *dst, const void *src, unsigned len)
{
	bcopy(src, dst, len);
}

int
doc2k_memcmp(const void *dst, const void *src, unsigned len)
{
	return (bcmp(src, dst, len));
}

void
doc2k_memset(void *dst, int c, unsigned len)
{
	u_char *p = dst;
	while (len--)
		*p++ = c;
}

static struct fla_s {
	int busy;
	int unit;
	unsigned nsect;
	struct doc2k_stat ds;
	struct bio_queue_head bio_queue;
	struct devstat stats;
	struct disk disk;
	cdev_t dev;
} softc[NFLA];

static int
flaopen(struct dev_open_args *ap)
{
	cdev_t dev = ap->a_head.a_dev;
	struct fla_s *sc;
	int error;
	struct disklabel *dl;

	if (fla_debug)
		printf("flaopen(%s %x %x)\n",
			devtoname(dev), ap->a_oflags, ap->a_devtype);

	sc = dev->si_drv1;

	error = doc2k_open(sc->unit);

	if (error) {
		printf("doc2k_open(%d) -> err %d\n", sc->unit, error);
		return (EIO);
	}

	dl = &sc->disk.d_label;
	bzero(dl, sizeof(*dl));
	error = doc2k_size(sc->unit, &dl->d_secperunit,
	    &dl->d_ncylinders, &dl->d_ntracks, &dl->d_nsectors);
	dl->d_secsize = DEV_BSIZE;
	dl->d_secpercyl = dl->d_ntracks * dl->d_nsectors; /* XXX */

	return (0);
}

static int
flaclose(struct dev_close_args *ap)
{
	cdev_t dev = ap->a_head.a_dev;
	int error;
	struct fla_s *sc;

	if (fla_debug)
		printf("flaclose(%s %x %x)\n",
			devtoname(dev), ap->a_fflag, ap->a_devtype);

	sc = dev->si_drv1;

	error = doc2k_close(sc->unit);
	if (error) {
		printf("doc2k_close(%d) -> err %d\n", sc->unit, error);
		return (EIO);
	}
	return (0);
}

static int
flaioctl(struct dev_ioctl_args *ap)
{
	cdev_t dev = ap->a_head.a_dev;

	if (fla_debug)
		printf("flaioctl(%s %lx %p %x)\n",
			devtoname(dev), ap->a_cmd, ap->a_data, ap->a_fflag);

	return (ENOIOCTL);
}

static int
flastrategy(struct dev_strategy_args *ap)
{
	cdev_t dev = ap->a_head.a_dev;
	struct bio *bio = ap->a_bio;
	struct buf *bp = bio->bio_buf;
	int unit, error;
	struct fla_s *sc;
	enum doc2k_work what;

	if (fla_debug > 1) {
		printf("flastrategy(%p) %s %x, %lld, %d, %p)\n",
			bp, devtoname(dev), bp->b_flags, bio->bio_offset, 
			bp->b_bcount, bp->b_data);
	}

	sc = dev->si_drv1;
	bio->bio_driver_info = dev;
	crit_enter();
	bioqdisksort(&sc->bio_queue, bio);
	if (sc->busy) {
		crit_exit();
		return(0);
	}
	sc->busy++;
	
	while (1) {
		bio = bioq_first(&sc->bio_queue);
		if (bio == NULL) {
			crit_exit();
			break;
		}
		bioq_remove(&sc->bio_queue, bio);
		bp = bio->bio_buf;
		dev = bio->bio_driver_info;

		devstat_start_transaction(&sc->stats);
		bp->b_resid = bp->b_bcount;
		unit = dkunit(dev);

		switch(bp->b_cmd) {
		case BUF_CMD_FREEBLKS:
			what = DOC2K_ERASE;
			break;
		case BUF_CMD_READ:
			what = DOC2K_READ;
			break;
		case BUF_CMD_WRITE:
			what = DOC2K_WRITE;
			break;
		default:
			panic("fla: bad b_cmd %d", bp->b_cmd);
		}

		LEAVE();

		error = doc2k_rwe(unit, what,
				  (unsigned)(bio->bio_offset >> DEV_BSHIFT),
				  bp->b_bcount / DEV_BSIZE, bp->b_data);

		ENTER();

		if (fla_debug > 1 || error) {
			printf("fla%d: %d = rwe(%p, %d, %d, %lld, %d, %p)\n",
			    unit, error, bp, unit, what, bio->bio_offset, 
			    bp->b_bcount, bp->b_data);
		}
		if (error) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
		} else {
			bp->b_resid = 0;
		}
		devstat_end_transaction_buf(&sc->stats, bp);
		biodone(bio);

		crit_enter();
	}
	sc->busy = 0;
	return(0);
}

static int
flaprobe (device_t dev)
{
	int unit;
	struct fla_s *sc;
	int i;

	unit = device_get_unit(dev);
	sc = &softc[unit];

	/* This is slightly ugly */
	i = doc2k_probe(unit, KERNBASE + 0xc0000, KERNBASE + 0xe0000);
	if (i)
		return (ENXIO);

	i = doc2k_info(unit, &sc->ds);
	if (i)
		return (ENXIO);

	bus_set_resource(dev, SYS_RES_MEMORY, 0, 
		sc->ds.window - KERNBASE, 8192);

	return (0);
}

static int
flaattach (device_t dev)
{
	int unit;
	int i, j, k, l, m, error;
	struct fla_s *sc;

	unit = device_get_unit(dev);
	sc = &softc[unit];

	error = doc2k_open(unit);
	if (error) {
		printf("doc2k_open(%d) -> err %d\n", unit, error);
		return (EIO);
	}

	error = doc2k_size(unit, &sc->nsect, &i, &j, &k );
	if (error) {
		printf("doc2k_size(%d) -> err %d\n", unit, error);
		return (EIO);
	}

	printf("fla%d: <%s %s>\n", unit, sc->ds.product, sc->ds.model);

	error = doc2k_close(unit);
	if (error) {
		printf("doc2k_close(%d) -> err %d\n", unit, error);
		return (EIO);
	}
	
	m = 1024L * 1024L / DEV_BSIZE;
	l = (sc->nsect * 10 + m/2) / m;
	printf("fla%d: %d.%01dMB (%u sectors),"
	    " %d cyls, %d heads, %d S/T, 512 B/S\n",
	    unit, l / 10, l % 10, sc->nsect, i, j, k);

	if (bootverbose)
		printf("fla%d: JEDEC=0x%x unitsize=%ld mediasize=%ld"
		       " chipsize=%ld interleave=%d window=%lx\n", 
		    unit, sc->ds.type, sc->ds.unitSize, sc->ds.mediaSize, 
		    sc->ds.chipSize, sc->ds.interleaving, sc->ds.window);

	bioq_init(&sc->bio_queue);

	devstat_add_entry(&softc[unit].stats, "fla", unit, DEV_BSIZE,
		DEVSTAT_NO_ORDERED_TAGS, 
		DEVSTAT_TYPE_DIRECT | DEVSTAT_TYPE_IF_OTHER,
		DEVSTAT_PRIORITY_DISK);

	sc->dev = disk_create(unit, &sc->disk, 0, &fla_ops);
	sc->dev->si_drv1 = sc;
	sc->unit = unit;

	return (0);
}

static device_method_t fla_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		flaprobe),
	DEVMETHOD(device_attach,	flaattach),
	{0, 0}
};

static driver_t fladriver = {
	"fla",
	fla_methods,
	sizeof(struct fla_s),
};

static devclass_t	fla_devclass;

DRIVER_MODULE(fla, isa, fladriver, fla_devclass, 0, 0);
