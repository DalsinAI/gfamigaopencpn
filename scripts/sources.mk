# What OpenCPN 5.14.2 builds from (the lists its CMake files give for Linux,
# less what AmigaOS 3 doesn't have; see README). Paths are inside its tree.
# Copyright (c) 2026 AmiCradle Team

MODEL_SRC := \
    ais_bitstring ais_decoder ais_state_vars ais_target_data autopilot_output base_platform \
    catalog_handler catalog_parser certificates chartdata_input_stream cli_platform cmdline \
    comm_ais comm_appmsg comm_appmsg_bus comm_bridge comm_buffers comm_can_util comm_decoder \
    comm_drv_factory comm_drv_file comm_drv_internal comm_drv_loopback comm_drv_n0183 \
    comm_drv_n0183_net comm_drv_n0183_serial comm_drv_n2k comm_drv_n2k_net comm_drv_n2k_serial \
    comm_drv_registry comm_drv_signalk comm_drv_signalk_net comm_n0183_output comm_navmsg \
    comm_navmsg_bus comm_out_queue comm_util comm_vars config_vars conn_params conn_states cutil \
    data_monitor_src datetime downloader ds_porttype filters_on_disk garmin_protocol_mgr geodesic \
    georef gpx_document gui_vars hyperlink ipc_api ipc_factories local_api logger mdns_cache \
    mdns_query mdns_service multiplexer nav_object_database navmsg_filter navobj_db \
    navobj_db_migrator navobj_db_util navutil_base notification notification_manager ocpn_plugin \
    ocpn_utils own_ship peer_client periodic_timer pincode plugin_api plugin_blacklist plugin_cache \
    plugin_comm plugin_handler plugin_loader plugin_paths position_parser rest_server route \
    route_point routeman safe_mode select select_item semantic_vers ser_ports std_icon \
    std_instance_chk svg_utils thread_ctrl track usb_watch_factory std_serial_io \

GUI_SRC := \
    about about_frame about_frame_impl ais ais_info_gui ais_target_alert_dlg ais_target_list_dlg \
    ais_target_query_dlg api_121 api_122 canvas_config canvas_menu canvas_options catalog_mgr \
    cat_settings chartdb chartdb_thread chartdbs chartimg chcanv ch_info_win cm93 color_handler \
    compass concanv config_mgr connection_edit connections_dlg conn_params_panel data_monitor \
    detail_slider displays download_mgr filter_dlg flex_hash font_desc font_mgr go_to_position_dlg \
    gshhs gui_lib hotkeys_dlg idx_entry ienc_toolbar kml layer link_prop_dlg load_errors_dlg \
    mark_info mbtiles/mbtiles mbtiles/tile_thread mbtiles/tile_cache mui_bar navutil \
    notification_manager_gui ocp_cursor ocpn_app ocpn_plugin_gui ocpn_aui_manager ocpndc ocpn_frame \
    ocpn_list_ctrl ocpn_pixel ocpn_platform ocpn_region ocpn_gl_options options o_senc \
    peer_client_dlg piano pluginmanager print_dialog printout_base printout_chart printtable \
    priority_gui quilt rest_server_gui rollover_win route_gui routemanagerdialog routeman_gui \
    route_point_gui route_printout route_validator route_prop_dlg route_prop_dlg_impl s57_load \
    s57obj s57chart s57_query_dlg safe_mode_gui senc_manager send_to_gps_dlg send_to_peer_dlg \
    shapefile_basemap station_data styles svg_icons tc_data_factory tc_data_source \
    tcds_ascii_harmonic tcds_binary_harmonic tcmgr tc_win thumbwin toolbar track_gui track_printout \
    track_prop_dlg tty_scroll udev_rule_mgr undo tooltip top_frame update_mgr user_colors_dlg \
    user_colors viewport waypointman_gui initwiz/wiz_ui_proto initwiz/wiz_ui ocpn_fontdlg \

LIB_SRC := \
    libs/ssl_sha1/src/sha1.c \
    $(addprefix libs/SQLiteCpp/src/,Backup.cpp Column.cpp Database.cpp Exception.cpp Savepoint.cpp Statement.cpp Transaction.cpp) \
    $(addprefix libs/ShapefileCpp/ShapeFileCpp-b2681f8/lib/src/,ShapefileReader.cpp Schema.cpp FeatureIterator.cpp \
        GeometryType.cpp Bounds.cpp Geometry.cpp Point.cpp Line.cpp Ring.cpp Polygon.cpp MultiPoint.cpp \
        MultiLine.cpp MultiPolygon.cpp Feature.cpp FieldType.cpp Field.cpp) \
    libs/serial/src/serial.cc \
    $(addprefix libs/libtess2/Source/,bucketalloc.c dict.c geom.c mesh.c priorityq.c sweep.c tess.c \
        Striper.cpp CustomArray.cpp Adjacency.cpp RevisitedRadix.cpp) \
    $(addprefix libs/tinyxml/src/,tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp) \
    $(addprefix libs/manual/src/,manual.cpp manual_dlg.cpp) \
    $(addprefix libs/s57-charts/src/,ogrs57datasource.cpp ogrs57layer.cpp s57classregistrar.cpp \
        s57featuredefns.cpp s57reader.cpp s57registrar_mgr.cpp) \
    $(addprefix libs/gui/src/,button_switch.cpp dialog_alert.cpp dialog_base.cpp edit_button.cpp \
        expand_icon.cpp expand_panel.cpp dialog_input.cpp field_switch.cpp field_text.cpp form_grid.cpp \
        svg_button.cpp svg_icons.cpp text_entry.cpp ui_utils.cpp) \
    $(addprefix libs/lz4/src/,lz4.c lz4hc.c) \
    $(addprefix libs/gdal/src/,cpl_conv.cpp cpl_csv.cpp cpl_error.cpp cpl_findfile.cpp cpl_minixml.cpp \
        cpl_path.cpp cpl_string.cpp cpl_vsisimple.cpp cplgetsymbol.cpp ograssemblepolygon.cpp ogrcurve.cpp \
        ogrfeature.cpp ogrfeaturedefn.cpp ogrfielddefn.cpp ogrgeometry.cpp ogrgeometrycollection.cpp \
        ogrgeometryfactory.cpp ogrlinearring.cpp ogrlinestring.cpp ogrmultilinestring.cpp ogrmultipoint.cpp \
        ogrmultipolygon.cpp ogrpoint.cpp ogrpolygon.cpp ogrlayer.cpp ogrutils.cpp gdal_misc.cpp) \
    $(addprefix libs/geoprim/src/,bbox.cpp LLRegion.cpp line_clip.cpp poly_math.cpp LOD_reduce.cpp) \
    $(addprefix libs/iso8211/src/,ddffield.cpp ddffielddefn.cpp ddfmodule.cpp ddfrecord.cpp \
        ddfrecordindex.cpp ddfsubfielddefn.cpp ddfutils.cpp) \
    $(addprefix libs/N2KParser/src/,N2KParser.cpp N2kMessages.cpp N2kMsg.cpp) \
    $(addprefix libs/nmea0183/src/,latlong.cpp long.cpp nmea0183.cpp response.cpp rmb.cpp sentence.cpp \
        talkerid.cpp rmc.cpp hexvalue.cpp lat.cpp expid.cpp wpl.cpp rte.cpp ths.cpp hdt.cpp hdg.cpp hdm.cpp \
        hvd.cpp gll.cpp vtg.cpp gga.cpp gsv.cpp GPwpl.cpp apb.cpp xte.cpp mwd.cpp mwv.cpp) \
    $(addprefix libs/observable/src/,observable_confvar.cpp observable.cpp) \
    libs/pugixml/pugixml.cpp \
    $(addprefix libs/s52plib/src/,chartsymbols.cpp s52plib.cpp s52cnsy.cpp s52utils.cpp TexFont.cpp \
        DepthFont.cpp mygeom.cpp s52_plib_utils.cpp) \
    libs/sqlite/src/sqlite3.c \
    $(addprefix libs/o_sound/src/,sound.cpp ocpn_wx_sound.cpp factory.cpp sound_file_loader.cpp \
        sound_loader_factory.cpp system_cmd_sound.cpp) \
    $(addprefix libs/wxJSON/src/,jsonreader.cpp jsonwriter.cpp jsonval.cpp)

SHP_SRC := shpopen.c dbfopen.c safileio.c sbnsearch.c shptree.c

# Not built yet (src/net_parked.cpp answers for them): downloads and sending
# routes to another OpenCPN (curl), the REST server's certificate (OpenSSL),
# mDNS. Signal K (IXWebSocket) and the REST server's io thread (mongoose) are
# compiled out by patches/0005. NMEA 0183 and NMEA 2000 over TCP/UDP are in,
# on the toolkit's sockets.
NONET_MODEL := certificates downloader peer_client mdns_query mdns_service comm_drv_signalk_net
NONET_GUI :=

# Amiga parts of this port (src/)
PORT_SRC := net_parked.cpp
# GLU's tesselator (OpenCPN's own copy, libs/glu), which its region and S-52
# code use with OpenGL on or off; the host check links the system's GLU
GLU_SRC :=
ifneq ($(HOST),1)
PORT_SRC += serial_amiga.cc gl_none.c libnix_extra.c
GLU_SRC := $(addprefix libs/glu/libtess/,tess.c mesh.c priorityq.c sweep.c tessmono.c normal.c \
    memalloc.c geom.c dict.c render.c) libs/glu/libutil/error.c libs/glu/libutil/glue.c
endif

OCPN_SRC := $(addprefix model/src/,$(addsuffix .cpp,$(filter-out $(NONET_MODEL),$(MODEL_SRC)))) \
    $(addprefix gui/src/,$(addsuffix .cpp,$(filter-out $(NONET_GUI),$(GUI_SRC)))) $(LIB_SRC) $(GLU_SRC)
