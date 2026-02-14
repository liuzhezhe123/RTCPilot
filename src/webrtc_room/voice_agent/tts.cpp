#include "tts.hpp"

#include "config/config.hpp"
#include "sherpa-onnx/c-api/cxx-api.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace cpp_streamer
{

TTSImpl::TTSImpl(Logger* logger) : logger_(logger) {
}

TTSImpl::~TTSImpl() {
    Release();
}

int TTSImpl::Init() {
    auto& voice_cfg = Config::Instance().voice_agent_cfg_;
    tts_enabled_ = voice_cfg.tts_config_.tts_enable_;
    if (!tts_enabled_) {
        LogInfof(logger_, "TTSImpl is disabled by configuration");
        return 0;
    }

    if (tts_) {
        return 0;
    }

    const auto& tts_cfg = voice_cfg.tts_config_;
    if (tts_cfg.acoustic_model_.empty() || tts_cfg.lexicon_.empty() ||
        tts_cfg.tokens_.empty()) {
        LogErrorf(logger_, "TTSImpl configuration is incomplete: acoustic_model=%s, lexicon=%s, tokens=%s",
                  tts_cfg.acoustic_model_.c_str(), tts_cfg.lexicon_.c_str(),
                  tts_cfg.tokens_.c_str());
        return -1;
    }

    LogInfof(logger_, "TTSImpl initializing with acoustic_model=%s,\r\nvocoder=%s,\r\n lexicon=%s,\r\n tokens=%s,\r\n dict_dir=%s,\r\n num_threads=%d",
             tts_cfg.acoustic_model_.c_str(), tts_cfg.vocoder_.c_str(),
             tts_cfg.lexicon_.c_str(), tts_cfg.tokens_.c_str(),
             tts_cfg.dict_dir_.c_str(), tts_cfg.num_threads_);
    sherpa_onnx::cxx::OfflineTtsConfig config;
    auto& matcha = config.model.matcha;
    matcha.acoustic_model = tts_cfg.acoustic_model_;
    matcha.vocoder = tts_cfg.vocoder_;
    matcha.lexicon = tts_cfg.lexicon_;
    matcha.tokens = tts_cfg.tokens_;
    matcha.dict_dir = tts_cfg.dict_dir_;
    config.model.num_threads = std::max<int32_t>(1, tts_cfg.num_threads_);
    config.model.provider = "cpu";
    config.model.debug = 0;

    try {
        auto offline_tts = sherpa_onnx::cxx::OfflineTts::Create(config);
        tts_.reset(new sherpa_onnx::cxx::OfflineTts(std::move(offline_tts)));
        sample_rate_ = tts_->SampleRate();
    } catch (const std::exception& e) {
        LogErrorf(logger_, "TTSImpl failed to create sherpa-onnx offline TTS: %s", e.what());
        return -1;
    }

    LogInfof(logger_, "TTSImpl initialized, sample_rate=%d", sample_rate_);
    return 0;
}

void TTSImpl::Release() {
    if (tts_) {
        tts_.reset();
        sample_rate_ = 0;
    }
    tts_enabled_ = false;
}

int TTSImpl::SynthesizeText(const std::string& text, int32_t& sample_rate,
                            std::vector<float>& audio_data) {
    sample_rate = 0;
    audio_data.clear();
    if (!tts_enabled_) {
        LogWarnf(logger_, "TTSImpl is disabled; cannot synthesize text");
        return -1;
    }
    if (!tts_) {
        LogWarnf(logger_, "TTSImpl is not initialized");
        return -1;
    }
    if (text.empty()) {
        LogWarnf(logger_, "TTSImpl invoked with empty text");
        return -1;
    }

    try {
        auto generated = tts_->Generate(text);
        sample_rate = generated.sample_rate;
        audio_data = std::move(generated.samples);
        return 0;
    } catch (const std::exception& e) {
        LogErrorf(logger_, "TTSImpl failed to synthesize text: %s", e.what());
        return -1;
    }
}

}