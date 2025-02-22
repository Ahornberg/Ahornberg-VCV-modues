#include "../../Ahornberg.hpp"

struct MIDIPolyExpressionData {
	float noteVolume;
	float notePitch;
	float volume;
	float oldVolume;
	float volumeMsb;
	float noteLength;
	float pitch;
	float modulation;
	float gate;
	float oldGate;
	bool volumeMsbSet;
};

struct MIDIPolyExpression : Module {
	constexpr static int MIN_MIDI_CHANNEL = 1;
	constexpr static int MAX_MIDI_CHANNEL = 16;
	constexpr static int SLEW_VALUE = 40;
	constexpr static int INITIAL_SLEW_VALUE = 10000;
	enum ParamIds {
		MIDI_CHANNEL_FIRST_PARAM,
		MIDI_CHANNEL_COUNT_PARAM,
		GATE_VELOCITY_MODE_PARAM,
		DECAY_PARAM,
		RELEASE_PARAM,
		PITCH_SHAPE_PARAM,
		VOLUME_SHAPE_PARAM,
		DECAY_Y_PARAM,
		PRESERVE_PITCH_AFTER_NOTEOFF_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		VOLUME_OUTPUT,
		PITCH_OUTPUT,
		MODULATION_OUTPUT,
		NOTE_OUTPUT,
		PITCHBEND_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		VOLUME_14_BIT_LIGHT,
		NUM_LIGHTS
	};

	MIDIPolyExpressionData envelopes[MAX_MIDI_CHANNEL];
	midi::InputQueue midiInput;
	dsp::SlewLimiter pitchSlews[MAX_MIDI_CHANNEL];
	dsp::ExponentialFilter modulationSlews[MAX_MIDI_CHANNEL];
	dsp::SlewLimiter volumeSlews[MAX_MIDI_CHANNEL];
	bool volume14BitMode;

	MIDIPolyExpression();
	void process(const ProcessArgs& args) override;
	void processMidiMessage(const midi::Message& msg);
	void onReset() override;
	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
};
