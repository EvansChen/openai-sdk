#pragma once

/**
 * Voice processing and audio workflows
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <map>
#include <any>

namespace openai_agents {
namespace voice {

// Audio formats
enum class AudioFormat {
    WAV,
    MP3,
    FLAC,
    OGG,
    M4A,
    PCM
};

// Audio configuration
struct AudioConfig {
    AudioFormat format = AudioFormat::WAV;
    int sample_rate = 44100;
    int channels = 1;
    int bit_depth = 16;
    std::map<std::string, std::any> metadata;
};

// Audio data container
class AudioData {
private:
    std::vector<uint8_t> data_;
    AudioConfig config_;
    std::chrono::milliseconds duration_;

public:
    AudioData(const std::vector<uint8_t>& data, const AudioConfig& config);
    
    // Data access
    const std::vector<uint8_t>& get_data() const { return data_; }
    const AudioConfig& get_config() const { return config_; }
    std::chrono::milliseconds get_duration() const { return duration_; }
    size_t get_size() const { return data_.size(); }
    
    // Utility
    bool is_empty() const { return data_.empty(); }
    void clear() { data_.clear(); duration_ = std::chrono::milliseconds(0); }
    
    // File operations
    void save_to_file(const std::string& filename) const;
    static std::shared_ptr<AudioData> load_from_file(const std::string& filename);

private:
    std::chrono::milliseconds calculate_duration() const;
};

// Voice input interface
class VoiceInput {
public:
    virtual ~VoiceInput() = default;
    
    // Recording control
    virtual void start_recording() = 0;
    virtual void stop_recording() = 0;
    virtual bool is_recording() const = 0;
    
    // Audio data
    virtual std::shared_ptr<AudioData> get_recorded_audio() = 0;
    virtual void clear_audio() = 0;
    
    // Configuration
    virtual void set_audio_config(const AudioConfig& config) = 0;
    virtual AudioConfig get_audio_config() const = 0;
    
    // Callbacks
    virtual void set_audio_callback(std::function<void(std::shared_ptr<AudioData>)> callback) = 0;
    virtual void set_error_callback(std::function<void(const std::string&)> callback) = 0;
};

// Microphone input implementation
class MicrophoneInput : public VoiceInput {
private:
    bool recording_;
    AudioConfig config_;
    std::shared_ptr<AudioData> current_audio_;
    std::function<void(std::shared_ptr<AudioData>)> audio_callback_;
    std::function<void(const std::string&)> error_callback_;

public:
    MicrophoneInput(const AudioConfig& config = AudioConfig());
    
    // VoiceInput interface
    void start_recording() override;
    void stop_recording() override;
    bool is_recording() const override { return recording_; }
    
    std::shared_ptr<AudioData> get_recorded_audio() override;
    void clear_audio() override;
    
    void set_audio_config(const AudioConfig& config) override { config_ = config; }
    AudioConfig get_audio_config() const override { return config_; }
    
    void set_audio_callback(std::function<void(std::shared_ptr<AudioData>)> callback) override;
    void set_error_callback(std::function<void(const std::string&)> callback) override;
    
    // Microphone-specific
    std::vector<std::string> list_available_devices() const;
    void set_device(const std::string& device_name);
    std::string get_current_device() const;

private:
    void record_audio_thread();
    void emit_audio(std::shared_ptr<AudioData> audio);
    void emit_error(const std::string& error);
};

// File input implementation
class FileVoiceInput : public VoiceInput {
private:
    std::string filename_;
    std::shared_ptr<AudioData> audio_data_;
    AudioConfig config_;
    bool simulating_recording_;

public:
    FileVoiceInput(const std::string& filename);
    
    // VoiceInput interface
    void start_recording() override;
    void stop_recording() override;
    bool is_recording() const override { return simulating_recording_; }
    
    std::shared_ptr<AudioData> get_recorded_audio() override;
    void clear_audio() override;
    
    void set_audio_config(const AudioConfig& config) override { config_ = config; }
    AudioConfig get_audio_config() const override { return config_; }
    
    void set_audio_callback(std::function<void(std::shared_ptr<AudioData>)> callback) override;
    void set_error_callback(std::function<void(const std::string&)> callback) override;
    
    // File-specific
    void load_file(const std::string& filename);
    bool is_file_loaded() const { return audio_data_ != nullptr; }

private:
    std::function<void(std::shared_ptr<AudioData>)> audio_callback_;
    std::function<void(const std::string&)> error_callback_;
};

// Voice processing events
enum class VoiceEventType {
    RecordingStarted,
    RecordingEnded,
    AudioAvailable,
    SpeechDetected,
    SilenceDetected,
    Error
};

struct VoiceEvent {
    VoiceEventType type;
    std::chrono::system_clock::time_point timestamp;
    std::shared_ptr<AudioData> audio_data;
    std::string message;
    std::map<std::string, std::any> metadata;
    
    VoiceEvent(VoiceEventType type, const std::string& message = "");
};

// Voice event handler
using VoiceEventHandler = std::function<void(const VoiceEvent&)>;

// Voice processor for advanced audio analysis
class VoiceProcessor {
private:
    std::vector<VoiceEventHandler> event_handlers_;
    AudioConfig config_;
    bool processing_enabled_;

public:
    VoiceProcessor(const AudioConfig& config = AudioConfig());
    
    // Event handling
    void add_event_handler(VoiceEventHandler handler);
    void remove_all_handlers();
    
    // Processing control
    void enable_processing() { processing_enabled_ = true; }
    void disable_processing() { processing_enabled_ = false; }
    bool is_processing_enabled() const { return processing_enabled_; }
    
    // Audio analysis
    void process_audio(std::shared_ptr<AudioData> audio);
    bool detect_speech(std::shared_ptr<AudioData> audio);
    bool detect_silence(std::shared_ptr<AudioData> audio);
    double get_volume_level(std::shared_ptr<AudioData> audio);
    
    // Configuration
    void set_audio_config(const AudioConfig& config) { config_ = config; }
    AudioConfig get_audio_config() const { return config_; }
    
    // Speech detection parameters
    void set_speech_threshold(double threshold);
    void set_silence_threshold(double threshold);
    void set_minimum_speech_duration(std::chrono::milliseconds duration);

private:
    void emit_event(const VoiceEvent& event);
    double calculate_rms(std::shared_ptr<AudioData> audio);
    
    // Thresholds
    double speech_threshold_ = 0.1;
    double silence_threshold_ = 0.05;
    std::chrono::milliseconds min_speech_duration_ = std::chrono::milliseconds(500);
};

// Voice utilities
class VoiceUtils {
public:
    // Format conversion
    static std::shared_ptr<AudioData> convert_format(std::shared_ptr<AudioData> audio, const AudioConfig& target_config);
    static std::vector<uint8_t> convert_to_format(const std::vector<uint8_t>& data, AudioFormat from, AudioFormat to);
    
    // Audio manipulation
    static std::shared_ptr<AudioData> trim_silence(std::shared_ptr<AudioData> audio, double threshold = 0.01);
    static std::shared_ptr<AudioData> normalize_volume(std::shared_ptr<AudioData> audio);
    static std::shared_ptr<AudioData> resample(std::shared_ptr<AudioData> audio, int target_sample_rate);
    
    // Analysis
    static double calculate_volume(std::shared_ptr<AudioData> audio);
    static std::chrono::milliseconds calculate_duration(std::shared_ptr<AudioData> audio);
    static bool is_valid_audio(std::shared_ptr<AudioData> audio);
    
    // File operations
    static std::string get_file_extension(AudioFormat format);
    static AudioFormat detect_format_from_file(const std::string& filename);
    static AudioConfig get_default_config(AudioFormat format);
};

// Voice input factory
class VoiceInputFactory {
public:
    static std::shared_ptr<VoiceInput> create_microphone_input(const AudioConfig& config = AudioConfig());
    static std::shared_ptr<VoiceInput> create_file_input(const std::string& filename);
    static std::shared_ptr<VoiceInput> create_default_input(const AudioConfig& config = AudioConfig());
    
    // Device enumeration
    static std::vector<std::string> list_available_microphones();
    static bool is_microphone_available();
};

} // namespace voice
} // namespace openai_agents