
LIB_VERSION = 0.0.1
LD_VERSION = 0:1:0

PVERSION="1.0.13"

BUILD_CC = @BUILD_CC@

SUBDIRS = OpenIPMI sdrcomp @MARVELL_BMC_DIR@
DIST_SUBDIRS = OpenIPMI sdrcomp marvell-bmc

AM_CFLAGS = -Wall -Wsign-compare -I$(top_srcdir)/include -DIPMI_CHECK_LOCKS \
	$(OPENSSLINCS) -DPVERSION="\"$(PVERSION)\""

AM_CPPFLAGS = -DSYSCONFDIR='"$(sysconfdir)"' -DSTATEDIR='"$(localstatedir)"'

lib_LTLIBRARIES = libIPMIlanserv.la

bin_PROGRAMS = ipmi_sim

noinst_PROGRAMS = ipmi_checksum

noinst_HEADERS = emu.h bmc.h

libIPMIlanserv_la_SOURCES = lanserv_ipmi.c lanserv_asf.c priv_table.c \
	lanserv_oem_force.c lanserv_config.c config.c serv.c serial_ipmi.c \
	persist.c extcmd.c
libIPMIlanserv_la_LIBADD = $(OPENSSLLIBS) -ldl
libIPMIlanserv_la_LDFLAGS = -version-info $(LD_VERSION) \
	-Wl,-Map -Wl,libIPMIlanserv.map \
	../utils/libOpenIPMIutils.la

ipmi_checksum_SOURCES = ipmi_checksum.c

ipmi_sim_SOURCES = ipmi_sim.c bmc.c emu_cmd.c sol.c \
	bmc_storage.c bmc_app.c bmc_chassis.c bmc_transport.c \
	bmc_sensor.c bmc_picmg.c
ipmi_sim_LDADD = $(POPTLIBS) libIPMIlanserv.la -lpthread
ipmi_sim_LDFLAGS = -rdynamic ../unix/libOpenIPMIposix.la \
	../utils/libOpenIPMIutils.la

man_MANS = ipmi_sim.1 ipmi_sim_cmd.5

EXTRA_DIST = README.vm $(man_MANS)

install-data-local:
	$(INSTALL) -m 755 -d "$(DESTDIR)$(sysconfdir)/ipmi/";

uninstall-local:
	-rmdir "$(DESTDIR)$(sysconfdir)/ipmi" 2>/dev/null

CLEANFILES = libIPMIlanserv.map
