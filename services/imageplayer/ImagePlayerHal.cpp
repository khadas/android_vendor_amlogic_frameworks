/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "ImagePlayerd"
#define LOG_NDEBUG 0
#include <string>
#include <inttypes.h>
#include <utils/String8.h>
#include "ImagePlayerHal.h"
#include <utils/Log.h>

namespace vendor {
    namespace amlogic {
        namespace hardware {
            namespace imageserver {
                namespace V1_0 {
                    namespace implementation {
                        ImagePlayerHal::ImagePlayerHal(): mDeathRecipient(new DeathRecipient(this)) {
                            // mImagePlayer = ImagePlayerService::instantiate();
                            mImagePlayer = new ImagePlayerService();
                        }

                        ImagePlayerHal::~ImagePlayerHal() {
                            delete mImagePlayer;
                        }

                        Return<Result>  ImagePlayerHal::init() {
                            ALOGE("ImagePlayerHal init");

                            if ( NULL != mImagePlayer) {
                                mImagePlayer->init();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result>  ImagePlayerHal::setDataSource(const hidl_string& uri) {
                            ALOGE("ImagePlayerHal setDataSource");

                            if ( NULL != mImagePlayer) {
                                std::string uristr = uri;
                                mImagePlayer->setDataSource(uristr.c_str());
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result>  ImagePlayerHal::setSampleSurfaceSize(int32_t sampleSize,
                                int32_t surfaceW, int32_t surfaceH) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setSampleSurfaceSize(sampleSize, surfaceW, surfaceH);
                            }

                            return Result::FAIL;
                        }
                        Return<Result>  ImagePlayerHal::setRotate(float degrees, int32_t autoCrop) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setRotate(degrees, autoCrop);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result>  ImagePlayerHal::setScale(float sx, float sy, int32_t autoCrop) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setScale(sx, sy, autoCrop);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result> ImagePlayerHal::setHWScale(float sc) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setHWScale(sc);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result> ImagePlayerHal::setRotateScale(float degrees, float sx, float sy,
                                int32_t autoCrop) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setRotateScale(degrees, sx, sy, autoCrop);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result> ImagePlayerHal::setTranslate(float tx, float ty) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setTranslate(tx, ty);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        Return<Result> ImagePlayerHal::setCropRect(int32_t cropX, int32_t cropY,
                                int32_t cropWidth, int32_t cropHeight) {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->setCropRect(cropX, cropY, cropWidth, cropHeight);
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::prepareBuf(const hidl_string& uri) {
                            if ( NULL != mImagePlayer) {
                                std::string uristr = uri;
                                mImagePlayer->prepareBuf(uristr.c_str());
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::showBuf() {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->showBuf();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::start() {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->start();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::prepare() {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->prepare();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::show() {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->show();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }

                        Return<Result> ImagePlayerHal::release() {
                            if ( NULL != mImagePlayer) {
                                mImagePlayer->release();
                                return Result::OK;
                            }

                            return Result::FAIL;
                        }
                        void ImagePlayerHal::handleServiceDeath(uint32_t cookie) {

                        }
                        ImagePlayerHal::DeathRecipient::DeathRecipient(sp<ImagePlayerHal> sch):
                            mImagePlayerHal(sch) {}
                        void ImagePlayerHal::DeathRecipient::serviceDied(uint64_t cookie,
                                const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
                            ALOGE("imageplayerservice daemon client died cookie:%d", (int)cookie);
                            uint32_t type = static_cast<uint32_t>(cookie);
                            mImagePlayerHal->handleServiceDeath(type);
                        }

                    } //namespace implementation
                }//namespace V1_0
            } //namespace imageserver
        }//namespace hardware
    } //namespace android
} //namespace vendor
