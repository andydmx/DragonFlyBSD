/*	$OpenBSD: qsphy.c,v 1.13 2005/02/19 06:00:04 brad Exp $	*/
/*	NetBSD: qsphy.c,v 1.19 2000/02/02 23:34:57 thorpej Exp 	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/mii/qsphy.c,v 1.1.2.2 2002/10/21 21:21:42 semenu Exp $
 */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * driver for Quality Semiconductor's QS6612 ethernet 10/100 PHY
 * datasheet from www.qualitysemi.com
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/module.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include "mii.h"
#include "miivar.h"
#include "miidevs.h"

#include "qsphyreg.h"

#include "miibus_if.h"

static int qsphy_probe		(device_t);
static int qsphy_attach		(device_t);

static device_method_t qsphy_methods[] = {
	/* device interface */
	DEVMETHOD(device_probe,		qsphy_probe),
	DEVMETHOD(device_attach,	qsphy_attach),
	DEVMETHOD(device_detach,	ukphy_detach),
	DEVMETHOD(device_shutdown,	bus_generic_shutdown),
	{ 0, 0 }
};

static const struct mii_phydesc qsphys[] = {
	MII_PHYDESC(QUALSEMI,	QS6612),
	MII_PHYDESC_NULL
};

static devclass_t qsphy_devclass;

static driver_t qsphy_driver = {
	"qsphy",
	qsphy_methods,
	sizeof(struct mii_softc)
};

DRIVER_MODULE(qsphy, miibus, qsphy_driver, qsphy_devclass, NULL, NULL);

static int	qsphy_service(struct mii_softc *, struct mii_data *, int);
static void	qsphy_reset(struct mii_softc *);
static void	qsphy_status(struct mii_softc *);

static int
qsphy_probe(device_t dev)
{
	struct mii_attach_args *ma = device_get_ivars(dev);
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, qsphys);
	if (mpd != NULL) {
		device_set_desc(dev, mpd->mpd_name);
		return (0);
	}
	return (ENXIO);
}

static int
qsphy_attach(device_t dev)
{
	struct mii_softc *sc;
	struct mii_attach_args *ma;
	struct mii_data *mii;

	sc = device_get_softc(dev);
	ma = device_get_ivars(dev);
	mii_softc_init(sc, ma);
	sc->mii_dev = device_get_parent(dev);
	mii = device_get_softc(sc->mii_dev);
	LIST_INSERT_HEAD(&mii->mii_phys, sc, mii_list);

	sc->mii_inst = mii->mii_instance;
	sc->mii_service = qsphy_service;
	sc->mii_reset = qsphy_reset;
	sc->mii_pdata = mii;
	sc->mii_flags |= MIIF_NOISOLATE;

	qsphy_reset(sc);

	mii->mii_instance++;

	sc->mii_capabilities = PHY_READ(sc, MII_BMSR) & ma->mii_capmask;
	device_printf(dev, " ");
	if (sc->mii_capabilities & BMSR_MEDIAMASK)
		mii_phy_add_media(sc);
	else
		kprintf("no media present");
	kprintf("\n");

	MIIBUS_MEDIAINIT(sc->mii_dev);
	return (0);
}

static int
qsphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	int reg;

	/*
	 * If we're not selected, then do nothing, just isolate, if
	 * changing media.
	 */
	if (IFM_INST(ife->ifm_media) != sc->mii_inst) {
		if (cmd == MII_MEDIACHG) {
			reg = PHY_READ(sc, MII_BMCR);
			PHY_WRITE(sc, MII_BMCR, reg | BMCR_ISO);
		}

		return (0);
	}

	switch (cmd) {
	case MII_POLLSTAT:
		break;

	case MII_MEDIACHG:
		/*
		 * If the interface is not up, don't do anything.
		 */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		mii_phy_set_media(sc);
		break;

	case MII_TICK:
		/*
		 * Is the interface even up?
		 */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			return (0);

		/*
		 * Only used for autonegotiation.
		 */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO)
			break;

		/*
		 * This PHY's autonegotiation doesn't need to be kicked.
		 */
		break;
	}

	/* Update the media status. */
	qsphy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return (0);
}

static void
qsphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	int bmsr, bmcr, pctl;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	bmsr = PHY_READ(sc, MII_BMSR) |
	    PHY_READ(sc, MII_BMSR);
	if (bmsr & BMSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	bmcr = PHY_READ(sc, MII_BMCR);
	if (bmcr & BMCR_ISO) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	if (bmcr & BMCR_AUTOEN) {
		if ((bmsr & BMSR_ACOMP) == 0) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}

		pctl = PHY_READ(sc, MII_QSPHY_PCTL);
		switch (pctl & PCTL_OPMASK) {
		case PCTL_10_T:
			mii->mii_media_active |= IFM_10_T;
			break;
		case PCTL_10_T_FDX:
			mii->mii_media_active |= IFM_10_T|IFM_FDX;
			break;
		case PCTL_100_TX:
			mii->mii_media_active |= IFM_100_TX;
			break;
		case PCTL_100_TX_FDX:
			mii->mii_media_active |= IFM_100_TX|IFM_FDX;
			break;
		case PCTL_100_T4:
			mii->mii_media_active |= IFM_100_T4;
			break;
		case PCTL_AN:
			mii->mii_media_active |= IFM_NONE;
			break;
		default:
			/* Erg... this shouldn't happen. */
			mii->mii_media_active |= IFM_NONE;
			break;
		}
	} else {
		mii->mii_media_active = mii->mii_media.ifm_cur->ifm_media;
	}
}

static void
qsphy_reset(struct mii_softc *sc)
{
	mii_phy_reset(sc);
	PHY_WRITE(sc, MII_QSPHY_IMASK, 0);
}
