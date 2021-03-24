/************************************************************************************
** Copyright (C), 2000-2013, Oplus Mobile Comm Corp., Ltd.
** All rights reserved.
**
** VENDOR_EDIT
**
** Description: -
**      mpeg1-3 encoder
**
**
** --------------------------- Revision History: --------------------------------
** <author>                              <data>     <version >  <desc>
** ------------------------------------------------------------------------------
** Zoufeng@Plf.Mediasrv.Player  2013/06/19   1.0        create file
** ------------------------------------------------------------------------------
**
************************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "MP3Encoder"
#include <utils/Log.h>
#include "MediaBufferGroup.h"
#include "foundation/ADebug.h"
#include "MediaDefs.h"
#include "MediaErrors.h"
#include "MetaData.h"

#include "libmp3lame/lame.h"
#include "MP3Encoder.h"

namespace android {

MP3Encoder::MP3Encoder(const sp<MediaSource> &source, const sp<MetaData> &meta)
    : mSource(source),
      mMeta(meta),
      mStarted(false),
      mBufferGroup(NULL),
      mInputBuffer(NULL),
      mEncoderHandle(NULL) {

      mLame = lame_init();

}

status_t MP3Encoder::initCheck() {

    CHECK(mMeta->findInt32(kKeySampleRate, &mSampleRate));
    CHECK(mMeta->findInt32(kKeyChannelCount, &mChannels));
    CHECK(mMeta->findInt32(kKeyBitRate, &mBitRate));
    ALOGV("mSampleRate %d mChannels %d mBitRate %d", mSampleRate, mChannels, mBitRate);
    return OK;
}

MP3Encoder::~MP3Encoder() {
    if (mStarted) {
        stop();
    }
}

status_t MP3Encoder::start(MetaData *params) {
    if (mStarted) {
        ALOGW("Call start() when encoder already started");
        return OK;
    }

    mBufferGroup = new MediaBufferGroup;
    mBufferGroup->add_buffer(new MediaBuffer(4096));

    if (OK, initCheck()) {
        return UNKNOWN_ERROR;
    }

    lame_set_num_channels(mLame, mChannels);
    lame_set_mode(mLame, mChannels > 1 ? JOINT_STEREO : MONO);
    lame_set_in_samplerate(mLame, mSampleRate);
    lame_set_out_samplerate(mLame, mSampleRate);
    lame_set_quality(mLame, 5);
    lame_set_brate(mLame, mBitRate);
    lame_set_bWriteVbrTag(mLame,0);
    int ipr = lame_init_params(mLame);

    mNumInputSamples = 0;
    mAnchorTimeUs = 0;
    mFrameCount = 0;
    if (mSource->start(params) != OK) {
        return UNKNOWN_ERROR;
    }

    mStarted = true;

    return OK;
}

status_t MP3Encoder::stop() {
    if (!mStarted) {
        ALOGW("Call stop() when encoder has not started");
        return OK;
    }

    if (mInputBuffer) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    delete mBufferGroup;
    mBufferGroup = NULL;
    mSource->stop();
    if (mLame != NULL) {
        lame_close(mLame);
    }
    return OK;
}

sp<MetaData> MP3Encoder::getFormat() {
    sp<MetaData> srcFormat = mSource->getFormat();

    mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);

    int64_t durationUs;
    if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
        mMeta->setInt64(kKeyDuration, durationUs);
    }

    mMeta->setCString(kKeyDecoderComponent, "MP3Encoder");

    return mMeta;
}

status_t MP3Encoder::read(
        MediaBuffer **out, const ReadOptions *options) {
    status_t err;
    bool readFromSource = false;
    int64_t wallClockTimeUs = -1;
    int nb_samples;
    int mp3_len;
    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    MediaBuffer *buffer;
    uint8_t *outPtr;

    CHECK(options == NULL || !options->getSeekTo(&seekTimeUs, &mode));
    *out = NULL;

    if (mBufferGroup->acquire_buffer(&buffer) != OK) {
        return UNKNOWN_ERROR;
    }

    outPtr = (uint8_t *)buffer->data();
    err = mSource->read(&mInputBuffer, options);
    if (err != OK) {
    #ifdef VENDOR_EDIT
    //Liang.Xu@MM.AudioServer.Policy, 2018/09/07, Add for soundrecorder err
        if (err == DEAD_OBJECT) {
            ALOGE("MP3Encoder mSource->read DEAD_OBJECT err 0x%x", err);
            buffer->release();
            buffer = NULL;
            if (mInputBuffer != NULL) {
                mInputBuffer->release();
                mInputBuffer = NULL;
            }
        }
    #endif /* VENDOR_EDIT */
        ALOGE("mSource->read err 0x%x", err);
        return err;
    }

    //ALOGV("mSource->read size %d ", mInputBuffer->range_length());
    nb_samples = mInputBuffer->range_length()/mChannels/2;

    if (mChannels > 1) {
        //mp3_len = lame_encode_buffer_interleaved(mLame, (short int*)(mInputBuffer->data()+mInputBuffer->range_offset()),
        mp3_len = lame_encode_buffer_interleaved(mLame, (short int*)mInputBuffer->data() + mInputBuffer->range_offset(),
                                              nb_samples, outPtr, buffer->size());
    } else {
        //mp3_len = lame_encode_buffer(mLame, (const short int*)(mInputBuffer->data()+mInputBuffer->range_offset()),
        mp3_len = lame_encode_buffer(mLame, (const short int*)mInputBuffer->data() + mInputBuffer->range_offset(),
                                              NULL, nb_samples, outPtr, buffer->size());
    }

    //ALOGV("lame_encode_buffer mp3_len %d ", mp3_len);


    mInputBuffer->release();
    mInputBuffer = NULL;

    if (mp3_len >= 0) {
        buffer->set_range(0, mp3_len);
    } else {
        ALOGE("lame_encode_buffer err");
        return UNKNOWN_ERROR;
    }
    int64_t mediaTimeUs =
        (mFrameCount * 1000000LL ) / mSampleRate;
    buffer->meta_data()->setInt64(kKeyTime, mAnchorTimeUs + mediaTimeUs);
    if (readFromSource && wallClockTimeUs != -1) {
        buffer->meta_data()->setInt64(kKeyDriftTime, mediaTimeUs - wallClockTimeUs);
    }

    mFrameCount += nb_samples;

    *out = buffer;
    return OK;
}

}  // namespace android
