#-------------------------------------------------
#
# Project created by QtCreator 2013-12-22T13:41:47
#
#-------------------------------------------------

QT       += core gui xml printsupport svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GalenQt
TEMPLATE = app
INCLUDEPATH += \
        libtiff \
        hdf5/src hdf5/hl/src

macx {
    ICON = Icon.icns
    DEFINES += EXTRA_FILE_DIALOG_OPTIONS=0
    DEFINES += EXTRA_COLOUR_DIALOG_OPTIONS=0
    DEFINES += cimg_display=0
    DEFINES += cimg_verbosity=1
    DEFINES += cimg_use_tiff
    SOURCES += libtiff/tif_unix.c
    SOURCES += hdf5/src/H5lib_settings.c hdf5/src/H5Tinit.c
    HEADERS += hdf5/src/H5pubconf.h hdf5/src/H5config.h
}
else:win32 {
    RC_FILE = app.rc
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += EXTRA_FILE_DIALOG_OPTIONS=0
    DEFINES += EXTRA_COLOUR_DIALOG_OPTIONS=0
    DEFINES += TIF_PLATFORM_CONSOLE
    DEFINES += cimg_display=0
    DEFINES += cimg_verbosity=1
    DEFINES += cimg_use_tiff
#    DEFINES += cimg_use_openmp
    DEFINES += _USE_MATH_DEFINES
    SOURCES += libtiff/tif_win32.c
    SOURCES += hdf5/src/H5lib_settings.c hdf5/src/H5Tinit.c
    HEADERS += hdf5/src/H5pubconf.h hdf5/src/H5config.h
#    QMAKE_CXXFLAGS += -bigobj -Wall
#    QMAKE_CXXFLAGS += -openmp
    QMAKE_CXXFLAGS_DEBUG += -Od -RTCsu
    QMAKE_CXXFLAGS_RELEASE += -Ox -fp:fast -GL
    QMAKE_LFLAGS_RELEASE += -LTCG
}
else:unix {
    DEFINES += EXTRA_FILE_DIALOG_OPTIONS=QFileDialog::DontUseNativeDialog
    DEFINES += EXTRA_COLOUR_DIALOG_OPTIONS=QColorDialog::DontUseNativeDialog
}

OBJECTS_DIR = obj
CONFIG += no_batch # this gets around a bug in Visual Studio with the object_parallel_to_source option
CONFIG += object_parallel_to_source # this is important to stop obj files overwriting each other

SOURCES += main.cpp\
        MainWindow.cpp \
    GraphicsView.cpp \
    AboutDialog.cpp \
    MdiChild.cpp \
    PreferencesDialog.cpp \
    MultiSpectralDocument.cpp \
    SingleChannelImage.cpp \
    HistogramDisplayWidget.cpp \
    GeometryEngine.cpp \
    ImageTreeWidgetItem.cpp \
    CustomScroller.cpp \
    RecipesDialog.cpp \
    LabelledPoints.cpp \
    LabelledPointsTreeWidgetItem.cpp \
    StrokeFont.cpp \
    libtiff/tif_stream.cxx \
    libtiff/tif_aux.c \
    libtiff/tif_close.c \
    libtiff/tif_codec.c \
    libtiff/tif_color.c \
    libtiff/tif_compress.c \
    libtiff/tif_dir.c \
    libtiff/tif_dirinfo.c \
    libtiff/tif_dirread.c \
    libtiff/tif_dirwrite.c \
    libtiff/tif_dumpmode.c \
    libtiff/tif_error.c \
    libtiff/tif_extension.c \
    libtiff/tif_fax3.c \
    libtiff/tif_fax3sm.c \
    libtiff/tif_flush.c \
    libtiff/tif_getimage.c \
    libtiff/tif_jbig.c \
    libtiff/tif_jpeg.c \
    libtiff/tif_jpeg_12.c \
    libtiff/tif_luv.c \
    libtiff/tif_lzma.c \
    libtiff/tif_lzw.c \
    libtiff/tif_next.c \
    libtiff/tif_ojpeg.c \
    libtiff/tif_open.c \
    libtiff/tif_packbits.c \
    libtiff/tif_pixarlog.c \
    libtiff/tif_predict.c \
    libtiff/tif_print.c \
    libtiff/tif_read.c \
    libtiff/tif_strip.c \
    libtiff/tif_swab.c \
    libtiff/tif_thunder.c \
    libtiff/tif_tile.c \
    libtiff/tif_version.c \
    libtiff/tif_warning.c \
    libtiff/tif_write.c \
    libtiff/tif_zip.c \
    PCA.cpp \
    PCADialog.cpp \
    LDA.cpp \
    LDADialog.cpp \
    Settings.cpp \
    hdf5/src/H5.c \
    hdf5/src/H5A.c \
    hdf5/src/H5Abtree2.c \
    hdf5/src/H5AC.c \
    hdf5/src/H5ACdbg.c \
    hdf5/src/H5AClog.c \
    hdf5/src/H5ACmpio.c \
    hdf5/src/H5ACproxy_entry.c \
    hdf5/src/H5Adense.c \
    hdf5/src/H5Adeprec.c \
    hdf5/src/H5Aint.c \
    hdf5/src/H5Atest.c \
    hdf5/src/H5B.c \
    hdf5/src/H5B2.c \
    hdf5/src/H5B2cache.c \
    hdf5/src/H5B2dbg.c \
    hdf5/src/H5B2hdr.c \
    hdf5/src/H5B2int.c \
    hdf5/src/H5B2internal.c \
    hdf5/src/H5B2leaf.c \
    hdf5/src/H5B2stat.c \
    hdf5/src/H5B2test.c \
    hdf5/src/H5Bcache.c \
    hdf5/src/H5Bdbg.c \
    hdf5/src/H5C.c \
    hdf5/src/H5Cdbg.c \
    hdf5/src/H5Cepoch.c \
    hdf5/src/H5checksum.c \
    hdf5/src/H5Cimage.c \
    hdf5/src/H5Clog.c \
    hdf5/src/H5Cmpio.c \
    hdf5/src/H5Cprefetched.c \
    hdf5/src/H5Cquery.c \
    hdf5/src/H5CS.c \
    hdf5/src/H5Ctag.c \
    hdf5/src/H5Ctest.c \
    hdf5/src/H5D.c \
    hdf5/src/H5dbg.c \
    hdf5/src/H5Dbtree.c \
    hdf5/src/H5Dbtree2.c \
    hdf5/src/H5Dchunk.c \
    hdf5/src/H5Dcompact.c \
    hdf5/src/H5Dcontig.c \
    hdf5/src/H5Ddbg.c \
    hdf5/src/H5Ddeprec.c \
    hdf5/src/H5Dearray.c \
    hdf5/src/H5Defl.c \
    hdf5/src/H5Dfarray.c \
    hdf5/src/H5Dfill.c \
    hdf5/src/H5Dint.c \
    hdf5/src/H5Dio.c \
    hdf5/src/H5Dlayout.c \
    hdf5/src/H5Dmpio.c \
    hdf5/src/H5Dnone.c \
    hdf5/src/H5Doh.c \
    hdf5/src/H5Dscatgath.c \
    hdf5/src/H5Dselect.c \
    hdf5/src/H5Dsingle.c \
    hdf5/src/H5Dtest.c \
    hdf5/src/H5Dvirtual.c \
    hdf5/src/H5E.c \
    hdf5/src/H5EA.c \
    hdf5/src/H5EAcache.c \
    hdf5/src/H5EAdbg.c \
    hdf5/src/H5EAdblkpage.c \
    hdf5/src/H5EAdblock.c \
    hdf5/src/H5EAhdr.c \
    hdf5/src/H5EAiblock.c \
    hdf5/src/H5EAint.c \
    hdf5/src/H5EAsblock.c \
    hdf5/src/H5EAstat.c \
    hdf5/src/H5EAtest.c \
    hdf5/src/H5Edeprec.c \
    hdf5/src/H5Eint.c \
    hdf5/src/H5F.c \
    hdf5/src/H5FA.c \
    hdf5/src/H5FAcache.c \
    hdf5/src/H5Faccum.c \
    hdf5/src/H5FAdbg.c \
    hdf5/src/H5FAdblkpage.c \
    hdf5/src/H5FAdblock.c \
    hdf5/src/H5FAhdr.c \
    hdf5/src/H5FAint.c \
    hdf5/src/H5FAstat.c \
    hdf5/src/H5FAtest.c \
    hdf5/src/H5Fcwfs.c \
    hdf5/src/H5FD.c \
    hdf5/src/H5Fdbg.c \
    hdf5/src/H5FDcore.c \
    hdf5/src/H5FDdirect.c \
    hdf5/src/H5Fdeprec.c \
    hdf5/src/H5FDfamily.c \
    hdf5/src/H5FDint.c \
    hdf5/src/H5FDlog.c \
    hdf5/src/H5FDmpi.c \
    hdf5/src/H5FDmpio.c \
    hdf5/src/H5FDmulti.c \
    hdf5/src/H5FDsec2.c \
    hdf5/src/H5FDspace.c \
    hdf5/src/H5FDstdio.c \
    hdf5/src/H5FDtest.c \
    hdf5/src/H5FDwindows.c \
    hdf5/src/H5Fefc.c \
    hdf5/src/H5Ffake.c \
    hdf5/src/H5Fint.c \
    hdf5/src/H5Fio.c \
    hdf5/src/H5FL.c \
    hdf5/src/H5Fmount.c \
    hdf5/src/H5Fmpi.c \
    hdf5/src/H5FO.c \
    hdf5/src/H5Fquery.c \
    hdf5/src/H5FS.c \
    hdf5/src/H5FScache.c \
    hdf5/src/H5FSdbg.c \
    hdf5/src/H5Fsfile.c \
    hdf5/src/H5FSint.c \
    hdf5/src/H5Fspace.c \
    hdf5/src/H5FSsection.c \
    hdf5/src/H5FSstat.c \
    hdf5/src/H5FStest.c \
    hdf5/src/H5Fsuper.c \
    hdf5/src/H5Fsuper_cache.c \
    hdf5/src/H5Ftest.c \
    hdf5/src/H5G.c \
    hdf5/src/H5Gbtree2.c \
    hdf5/src/H5Gcache.c \
    hdf5/src/H5Gcompact.c \
    hdf5/src/H5Gdense.c \
    hdf5/src/H5Gdeprec.c \
    hdf5/src/H5Gent.c \
    hdf5/src/H5Gint.c \
    hdf5/src/H5Glink.c \
    hdf5/src/H5Gloc.c \
    hdf5/src/H5Gname.c \
    hdf5/src/H5Gnode.c \
    hdf5/src/H5Gobj.c \
    hdf5/src/H5Goh.c \
    hdf5/src/H5Groot.c \
    hdf5/src/H5Gstab.c \
    hdf5/src/H5Gtest.c \
    hdf5/src/H5Gtraverse.c \
    hdf5/src/H5HF.c \
    hdf5/src/H5HFbtree2.c \
    hdf5/src/H5HFcache.c \
    hdf5/src/H5HFdbg.c \
    hdf5/src/H5HFdblock.c \
    hdf5/src/H5HFdtable.c \
    hdf5/src/H5HFhdr.c \
    hdf5/src/H5HFhuge.c \
    hdf5/src/H5HFiblock.c \
    hdf5/src/H5HFiter.c \
    hdf5/src/H5HFman.c \
    hdf5/src/H5HFsection.c \
    hdf5/src/H5HFspace.c \
    hdf5/src/H5HFstat.c \
    hdf5/src/H5HFtest.c \
    hdf5/src/H5HFtiny.c \
    hdf5/src/H5HG.c \
    hdf5/src/H5HGcache.c \
    hdf5/src/H5HGdbg.c \
    hdf5/src/H5HGquery.c \
    hdf5/src/H5HL.c \
    hdf5/src/H5HLcache.c \
    hdf5/src/H5HLdbg.c \
    hdf5/src/H5HLdblk.c \
    hdf5/src/H5HLint.c \
    hdf5/src/H5HLprfx.c \
    hdf5/src/H5HP.c \
    hdf5/src/H5I.c \
    hdf5/src/H5Itest.c \
    hdf5/src/H5L.c \
    hdf5/src/H5Lexternal.c \
    hdf5/src/H5MF.c \
    hdf5/src/H5MFaggr.c \
    hdf5/src/H5MFdbg.c \
    hdf5/src/H5MFsection.c \
    hdf5/src/H5MM.c \
    hdf5/src/H5MP.c \
    hdf5/src/H5MPtest.c \
    hdf5/src/H5O.c \
    hdf5/src/H5Oainfo.c \
    hdf5/src/H5Oalloc.c \
    hdf5/src/H5Oattr.c \
    hdf5/src/H5Oattribute.c \
    hdf5/src/H5Obogus.c \
    hdf5/src/H5Obtreek.c \
    hdf5/src/H5Ocache.c \
    hdf5/src/H5Ocache_image.c \
    hdf5/src/H5Ochunk.c \
    hdf5/src/H5Ocont.c \
    hdf5/src/H5Ocopy.c \
    hdf5/src/H5Odbg.c \
    hdf5/src/H5Odrvinfo.c \
    hdf5/src/H5Odtype.c \
    hdf5/src/H5Oefl.c \
    hdf5/src/H5Ofill.c \
    hdf5/src/H5Oflush.c \
    hdf5/src/H5Ofsinfo.c \
    hdf5/src/H5Oginfo.c \
    hdf5/src/H5Oint.c \
    hdf5/src/H5Olayout.c \
    hdf5/src/H5Olinfo.c \
    hdf5/src/H5Olink.c \
    hdf5/src/H5Omessage.c \
    hdf5/src/H5Omtime.c \
    hdf5/src/H5Oname.c \
    hdf5/src/H5Onull.c \
    hdf5/src/H5Opline.c \
    hdf5/src/H5Orefcount.c \
    hdf5/src/H5Osdspace.c \
    hdf5/src/H5Oshared.c \
    hdf5/src/H5Oshmesg.c \
    hdf5/src/H5Ostab.c \
    hdf5/src/H5Otest.c \
    hdf5/src/H5Ounknown.c \
    hdf5/src/H5P.c \
    hdf5/src/H5Pacpl.c \
    hdf5/src/H5PB.c \
    hdf5/src/H5Pdapl.c \
    hdf5/src/H5Pdcpl.c \
    hdf5/src/H5Pdeprec.c \
    hdf5/src/H5Pdxpl.c \
    hdf5/src/H5Pencdec.c \
    hdf5/src/H5Pfapl.c \
    hdf5/src/H5Pfcpl.c \
    hdf5/src/H5Pfmpl.c \
    hdf5/src/H5Pgcpl.c \
    hdf5/src/H5Pint.c \
    hdf5/src/H5PL.c \
    hdf5/src/H5Plapl.c \
    hdf5/src/H5Plcpl.c \
    hdf5/src/H5PLint.c \
    hdf5/src/H5PLpath.c \
    hdf5/src/H5PLplugin_cache.c \
    hdf5/src/H5Pocpl.c \
    hdf5/src/H5Pocpypl.c \
    hdf5/src/H5Pstrcpl.c \
    hdf5/src/H5Ptest.c \
    hdf5/src/H5R.c \
    hdf5/src/H5Rdeprec.c \
    hdf5/src/H5Rint.c \
    hdf5/src/H5RS.c \
    hdf5/src/H5S.c \
    hdf5/src/H5Sall.c \
    hdf5/src/H5Sdbg.c \
    hdf5/src/H5Shyper.c \
    hdf5/src/H5SL.c \
    hdf5/src/H5SM.c \
    hdf5/src/H5SMbtree2.c \
    hdf5/src/H5SMcache.c \
    hdf5/src/H5SMmessage.c \
    hdf5/src/H5Smpio.c \
    hdf5/src/H5SMtest.c \
    hdf5/src/H5Snone.c \
    hdf5/src/H5Spoint.c \
    hdf5/src/H5Sselect.c \
    hdf5/src/H5ST.c \
    hdf5/src/H5Stest.c \
    hdf5/src/H5system.c \
    hdf5/src/H5T.c \
    hdf5/src/H5Tarray.c \
    hdf5/src/H5Tbit.c \
    hdf5/src/H5Tcommit.c \
    hdf5/src/H5Tcompound.c \
    hdf5/src/H5Tconv.c \
    hdf5/src/H5Tcset.c \
    hdf5/src/H5Tdbg.c \
    hdf5/src/H5Tdeprec.c \
    hdf5/src/H5Tenum.c \
    hdf5/src/H5Tfields.c \
    hdf5/src/H5Tfixed.c \
    hdf5/src/H5Tfloat.c \
    hdf5/src/H5timer.c \
    hdf5/src/H5Tnative.c \
    hdf5/src/H5Toffset.c \
    hdf5/src/H5Toh.c \
    hdf5/src/H5Topaque.c \
    hdf5/src/H5Torder.c \
    hdf5/src/H5Tpad.c \
    hdf5/src/H5Tprecis.c \
    hdf5/src/H5trace.c \
    hdf5/src/H5TS.c \
    hdf5/src/H5Tstrpad.c \
    hdf5/src/H5Tvisit.c \
    hdf5/src/H5Tvlen.c \
    hdf5/src/H5UC.c \
    hdf5/src/H5VM.c \
    hdf5/src/H5WB.c \
    hdf5/src/H5Z.c \
    hdf5/src/H5Zdeflate.c \
    hdf5/src/H5Zfletcher32.c \
    hdf5/src/H5Znbit.c \
    hdf5/src/H5Zscaleoffset.c \
    hdf5/src/H5Zshuffle.c \
    hdf5/src/H5Zszip.c \
    hdf5/src/H5Ztrans.c \
    hdf5/hl/src/H5DO.c \
    hdf5/hl/src/H5DS.c \
    hdf5/hl/src/H5IM.c \
    hdf5/hl/src/H5LD.c \
    hdf5/hl/src/H5LT.c \
    hdf5/hl/src/H5LTanalyze.c \
    hdf5/hl/src/H5LTparse.c \
    hdf5/hl/src/H5PT.c \
    hdf5/hl/src/H5TB.c \
    HDF5ReaderDialog.cpp \
    Utilities.cpp

HEADERS  += MainWindow.h \
    GraphicsView.h \
    AboutDialog.h \
    MdiChild.h \
    PreferencesDialog.h \
    MultiSpectralDocument.h \
    SingleChannelImage.h \
    HistogramDisplayWidget.h \
    GeometryEngine.h \
    ImageTreeWidgetItem.h \
    CustomScroller.h \
    RecipesDialog.h \
    LabelledPoints.h \
    LabelledPointsTreeWidgetItem.h \
    StrokeFont.h \
    CImg.h \
    libtiff/t4.h \
    libtiff/tif_config.h \
    libtiff/tif_dir.h \
    libtiff/tif_fax3.h \
    libtiff/tif_predict.h \
    libtiff/tiff.h \
    libtiff/tiffconf.h \
    libtiff/tiffio.h \
    libtiff/tiffio.hxx \
    libtiff/tiffiop.h \
    libtiff/tiffvers.h \
    libtiff/uvcode.h \
    PCA.h \
    PCADialog.h \
    LDA.h \
    LDADialog.h \
    Settings.h \
    hdf5/src/H5ACmodule.h \
    hdf5/src/H5ACpkg.h \
    hdf5/src/H5ACprivate.h \
    hdf5/src/H5ACpublic.h \
    hdf5/src/H5Amodule.h \
    hdf5/src/H5api_adpt.h \
    hdf5/src/H5Apkg.h \
    hdf5/src/H5Aprivate.h \
    hdf5/src/H5Apublic.h \
    hdf5/src/H5B2module.h \
    hdf5/src/H5B2pkg.h \
    hdf5/src/H5B2private.h \
    hdf5/src/H5B2public.h \
    hdf5/src/H5Bmodule.h \
    hdf5/src/H5Bpkg.h \
    hdf5/src/H5Bprivate.h \
    hdf5/src/H5Bpublic.h \
    hdf5/src/H5Cmodule.h \
    hdf5/src/H5Cpkg.h \
    hdf5/src/H5Cprivate.h \
    hdf5/src/H5Cpublic.h \
    hdf5/src/H5CSprivate.h \
    hdf5/src/H5Dmodule.h \
    hdf5/src/H5Dpkg.h \
    hdf5/src/H5Dprivate.h \
    hdf5/src/H5Dpublic.h \
    hdf5/src/H5EAmodule.h \
    hdf5/src/H5EApkg.h \
    hdf5/src/H5EAprivate.h \
    hdf5/src/H5Edefin.h \
    hdf5/src/H5Einit.h \
    hdf5/src/H5Emodule.h \
    hdf5/src/H5Epkg.h \
    hdf5/src/H5Eprivate.h \
    hdf5/src/H5Epubgen.h \
    hdf5/src/H5Epublic.h \
    hdf5/src/H5Eterm.h \
    hdf5/src/H5FAmodule.h \
    hdf5/src/H5FApkg.h \
    hdf5/src/H5FAprivate.h \
    hdf5/src/H5FDcore.h \
    hdf5/src/H5FDdirect.h \
    hdf5/src/H5FDdrvr_module.h \
    hdf5/src/H5FDfamily.h \
    hdf5/src/H5FDlog.h \
    hdf5/src/H5FDmodule.h \
    hdf5/src/H5FDmpi.h \
    hdf5/src/H5FDmpio.h \
    hdf5/src/H5FDmulti.h \
    hdf5/src/H5FDpkg.h \
    hdf5/src/H5FDprivate.h \
    hdf5/src/H5FDpublic.h \
    hdf5/src/H5FDsec2.h \
    hdf5/src/H5FDstdio.h \
    hdf5/src/H5FDwindows.h \
    hdf5/src/H5FLmodule.h \
    hdf5/src/H5FLprivate.h \
    hdf5/src/H5Fmodule.h \
    hdf5/src/H5FOprivate.h \
    hdf5/src/H5Fpkg.h \
    hdf5/src/H5Fprivate.h \
    hdf5/src/H5Fpublic.h \
    hdf5/src/H5FSmodule.h \
    hdf5/src/H5FSpkg.h \
    hdf5/src/H5FSprivate.h \
    hdf5/src/H5FSpublic.h \
    hdf5/src/H5Gmodule.h \
    hdf5/src/H5Gpkg.h \
    hdf5/src/H5Gprivate.h \
    hdf5/src/H5Gpublic.h \
    hdf5/src/H5HFmodule.h \
    hdf5/src/H5HFpkg.h \
    hdf5/src/H5HFprivate.h \
    hdf5/src/H5HFpublic.h \
    hdf5/src/H5HGmodule.h \
    hdf5/src/H5HGpkg.h \
    hdf5/src/H5HGprivate.h \
    hdf5/src/H5HGpublic.h \
    hdf5/src/H5HLmodule.h \
    hdf5/src/H5HLpkg.h \
    hdf5/src/H5HLprivate.h \
    hdf5/src/H5HLpublic.h \
    hdf5/src/H5HPprivate.h \
    hdf5/src/H5Imodule.h \
    hdf5/src/H5Ipkg.h \
    hdf5/src/H5Iprivate.h \
    hdf5/src/H5Ipublic.h \
    hdf5/src/H5Lmodule.h \
    hdf5/src/H5Lpkg.h \
    hdf5/src/H5Lprivate.h \
    hdf5/src/H5Lpublic.h \
    hdf5/src/H5MFmodule.h \
    hdf5/src/H5MFpkg.h \
    hdf5/src/H5MFprivate.h \
    hdf5/src/H5MMprivate.h \
    hdf5/src/H5MMpublic.h \
    hdf5/src/H5MPmodule.h \
    hdf5/src/H5MPpkg.h \
    hdf5/src/H5MPprivate.h \
    hdf5/src/H5Omodule.h \
    hdf5/src/H5Opkg.h \
    hdf5/src/H5Oprivate.h \
    hdf5/src/H5Opublic.h \
    hdf5/src/H5Oshared.h \
    hdf5/src/H5overflow.h \
    hdf5/src/H5PBmodule.h \
    hdf5/src/H5PBpkg.h \
    hdf5/src/H5PBprivate.h \
    hdf5/src/H5PLextern.h \
    hdf5/src/H5PLmodule.h \
    hdf5/src/H5PLpkg.h \
    hdf5/src/H5PLprivate.h \
    hdf5/src/H5PLpublic.h \
    hdf5/src/H5Pmodule.h \
    hdf5/src/H5Ppkg.h \
    hdf5/src/H5Pprivate.h \
    hdf5/src/H5Ppublic.h \
    hdf5/src/H5private.h \
    hdf5/src/H5public.h \
    hdf5/src/H5Rmodule.h \
    hdf5/src/H5Rpkg.h \
    hdf5/src/H5Rprivate.h \
    hdf5/src/H5Rpublic.h \
    hdf5/src/H5RSprivate.h \
    hdf5/src/H5SLmodule.h \
    hdf5/src/H5SLprivate.h \
    hdf5/src/H5SMmodule.h \
    hdf5/src/H5Smodule.h \
    hdf5/src/H5SMpkg.h \
    hdf5/src/H5SMprivate.h \
    hdf5/src/H5Spkg.h \
    hdf5/src/H5Sprivate.h \
    hdf5/src/H5Spublic.h \
    hdf5/src/H5STprivate.h \
    hdf5/src/H5Tmodule.h \
    hdf5/src/H5Tpkg.h \
    hdf5/src/H5Tprivate.h \
    hdf5/src/H5Tpublic.h \
    hdf5/src/H5TSprivate.h \
    hdf5/src/H5UCprivate.h \
    hdf5/src/H5version.h \
    hdf5/src/H5VMprivate.h \
    hdf5/src/H5WBprivate.h \
    hdf5/src/H5win32defs.h \
    hdf5/src/H5Zmodule.h \
    hdf5/src/H5Zpkg.h \
    hdf5/src/H5Zprivate.h \
    hdf5/src/H5Zpublic.h \
    hdf5/src/hdf5.h \
    hdf5/hl/src/H5DOpublic.h \
    hdf5/hl/src/H5DSprivate.h \
    hdf5/hl/src/H5DSpublic.h \
    hdf5/hl/src/H5HLprivate2.h \
    hdf5/hl/src/H5IMprivate.h \
    hdf5/hl/src/H5IMpublic.h \
    hdf5/hl/src/H5LDprivate.h \
    hdf5/hl/src/H5LDpublic.h \
    hdf5/hl/src/H5LTparse.h \
    hdf5/hl/src/H5LTprivate.h \
    hdf5/hl/src/H5LTpublic.h \
    hdf5/hl/src/H5PTprivate.h \
    hdf5/hl/src/H5PTpublic.h \
    hdf5/hl/src/H5TBprivate.h \
    hdf5/hl/src/H5TBpublic.h \
    hdf5/hl/src/hdf5_hl.h \
    HDF5ReaderDialog.h \
    Utilities.h

FORMS    += \
    AboutDialog.ui \
    MdiChild.ui \
    PCADialog.ui \
    LDADialog.ui \
    HDF5ReaderDialog.ui

RESOURCES += \
    mdi.qrc

OTHER_FILES += \
    app.rc
