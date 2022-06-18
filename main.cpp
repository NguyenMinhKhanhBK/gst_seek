#include "gst/gstbin.h"
#include "gst/gstbus.h"
#include "gst/gstclock.h"
#include "gst/gstelement.h"
#include "gst/gstelementfactory.h"
#include "gst/gstmessage.h"
#include "gst/gstobject.h"
#include "gst/gstpad.h"
#include "gst/gstpipeline.h"
#include "gst/gstutils.h"
#include <iostream>
#include <gst/gst.h>

static const char* MEDIA_PATH = "/home/khanhnguyen/media/video_10s.mp4";

struct CustomData {
    GstElement* pipeline;
    GstElement* filesrc;
    GstElement* decodebin;
    GstElement* videoconvert;
    GstElement* autovideosink;
};

void pad_added_handler(GstElement* src, GstPad* new_pad, gpointer user_data);

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv); 

    CustomData data;

    data.filesrc = gst_element_factory_make("filesrc", "file_src");
    if (data.filesrc == nullptr) {
        std::cout << "cannot create filesrc \n";
        return -1;
    }

    g_object_set(data.filesrc, "location", MEDIA_PATH, NULL);

    data.decodebin = gst_element_factory_make("decodebin", "decode_bin");
    if (data.decodebin == nullptr) {
        std::cout << "cannot create decodebin \n";
        return -1;
    }

    data.videoconvert = gst_element_factory_make("videoconvert", "video_convert");
    if (data.videoconvert == nullptr) {
        std::cout << "cannot create videoconvert \n";
        return -1;
    }

    data.autovideosink = gst_element_factory_make("autovideosink", "auto_video_sink");
    if (data.autovideosink == nullptr) {
        std::cout << "cannot create autovideosink \n";
        return -1;
    }


    data.pipeline = gst_pipeline_new("pipeline");
    if (data.pipeline == nullptr) {
        std::cout << "cannot create pipeline \n";
        return -1;
    }

    gst_bin_add_many(GST_BIN(data.pipeline), data.filesrc, data.decodebin, data.videoconvert, data.autovideosink, NULL);
    gst_element_link(data.filesrc, data.decodebin);
    gst_element_link(data.videoconvert, data.autovideosink);

    g_signal_connect(data.decodebin, "pad-added", G_CALLBACK(pad_added_handler), &data);

    auto ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cout << "failed to change pipeline to PLAYING state \n";
        return -1;
    }

    bool exit = false;
    auto bus = gst_element_get_bus(data.pipeline);
    do {
        auto msg = gst_bus_timed_pop_filtered(bus, 100*GST_MSECOND, GstMessageType(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (msg == nullptr) {
            continue;
        }

        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("error received from element %s: %s \n", GST_OBJECT_NAME(msg->src), err->message);
                g_clear_error(&err);
                g_free(debug_info);
                exit = true;
                break;

            case GST_MESSAGE_EOS:
                std::cout << "receive EOS signal, re-run \n";
                if (!gst_element_seek(data.pipeline,
                      1.0,
                      GST_FORMAT_TIME,
                      GST_SEEK_FLAG_FLUSH,
                      GST_SEEK_TYPE_SET,
                      1000000000,
                      GST_SEEK_TYPE_NONE,
                      GST_CLOCK_TIME_NONE)) {
                        std::cout << "failed to seek pipeline \n"; 
                        exit = true;
                        break;
                    }
                break;

            case GST_MESSAGE_STATE_CHANGED:
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    g_print("pipeline state changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                }
                break;

            default:
                g_printerr("unexpected message received \n");
                break;
        }
        gst_message_unref(msg);
    } while (!exit);





}

void pad_added_handler(GstElement* src, GstPad* new_pad, gpointer user_data) {
    CustomData* data = static_cast<CustomData*>(user_data);

    g_print("received new pad %s from %s\n",
            GST_PAD_NAME(new_pad),
            GST_ELEMENT_NAME(src));

    GstPad *video_convert_sink_pad, *video_convert_src_pad;
    GstPadLinkReturn ret;

    GstCaps* new_cap = gst_pad_get_current_caps(new_pad);
    GstStructure* new_pad_struct = gst_caps_get_structure(new_cap, 0);
    const gchar* new_pad_type = gst_structure_get_name(new_pad_struct);

    if (!g_str_has_prefix(new_pad_type, "video")) {
        std::cout << "ignore due to non video pad\n";
        gst_caps_unref(new_cap);
        return;
    }

    video_convert_sink_pad = gst_element_get_static_pad(data->videoconvert, "sink");
    video_convert_src_pad = gst_element_get_static_pad(data->videoconvert, "src");

    ret = gst_pad_link(new_pad, video_convert_sink_pad);
    if (GST_PAD_LINK_FAILED(ret)) {
        std::cout << "failed to link to video convert sink pad \n";
        goto exit_func;
    }

exit_func:
    gst_object_unref(video_convert_sink_pad);
    gst_object_unref(video_convert_src_pad);
    gst_caps_unref(new_cap);

}
