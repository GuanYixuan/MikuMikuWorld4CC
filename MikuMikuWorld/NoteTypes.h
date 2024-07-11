#pragma once
#include <stdint.h>
#include <stdexcept>

namespace MikuMikuWorld
{
	enum class NoteType : uint8_t
	{
		Tap,
		Hold,
		HoldMid,
		HoldEnd,
		Damage,
	};

	enum class FlickType : uint8_t
	{
		None,
		Default,
		Left,
		Right,
		FlickTypeCount
	};

	constexpr const char* flickTypes[]{ "none", "default", "left", "right" };

	enum class HoldStepType : uint8_t
	{
		Normal,
		Hidden,
		Skip,
		HoldStepTypeCount
	};

	constexpr const char* stepTypes[]{ "normal", "hidden", "skip" };

	// Ease type enum class providing the `ease` method
	class EaseType {
	  public:
		enum __Inner_type : uint8_t {
			Linear,
			EaseIn,
			EaseOut,
			EaseInOut,
			EaseOutIn,
			EaseTypeCount
		} __value;
		constexpr EaseType() noexcept : __value(__Inner_type::Linear) {}
		// Allow direct constructions
		constexpr EaseType(__Inner_type val) noexcept : __value(val) {}
		// Allow constructing from ints
		constexpr EaseType(int val) noexcept : __value(__Inner_type(val)) {}

		// Allow switches and comparisons
		constexpr operator __Inner_type() const noexcept { return __value; }
		// Prevent `if(ease_type)` usage
		explicit operator bool() const = delete;

		/**
		 * @brief "Ease" according to given `time_ratio` and current ease type
		 * @exception std::invalid_argument if this ease type is invalid
		 */
		float ease(float time_ratio) const {
			switch (__value) {
				case EaseType::Linear:
					return time_ratio;
				case EaseType::EaseIn:
					return time_ratio * time_ratio;
				case EaseType::EaseOut:
					return 1 - (1 - time_ratio) * (1 - time_ratio);
				case EaseType::EaseInOut:
					if (time_ratio < 0.5) return 2 * time_ratio * time_ratio;
					else return 1 - (1 - time_ratio) * (1 - time_ratio) * 2;
				case EaseType::EaseOutIn:
					return 0.5 + (0.5 - time_ratio) * (0.5 - time_ratio) * (time_ratio < 0.5 ? -2 : 2);
				default:
					throw std::invalid_argument("Invalid ease type");
			}
		}
	};

	constexpr const char* easeNames[]{ "linear", "in", "out", "inout", "outin" };

	constexpr const char* easeTypes[]{ "linear", "ease_in", "ease_out", "ease_in_out",
		                               "ease_out_in" };

	enum class HoldNoteType : uint8_t
	{
		Normal,
		Hidden,
		Guide
	};

	constexpr const char* holdTypes[]{ "normal", "hidden", "guide" };

	enum class GuideColor : uint8_t
	{
		Neutral,
		Red,
		Green,
		Blue,
		Yellow,
		Purple,
		Cyan,
		Black,
		GuideColorCount
	};

	constexpr const char* guideColors[]{ "neutral", "red",    "green", "blue",
		                                 "yellow",  "purple", "cyan",  "black" };
	constexpr const char* guideColorsForString[]{ "guide_neutral", "guide_red",    "guide_green",
		                                          "guide_blue",    "guide_yellow", "guide_purple",
		                                          "guide_cyan",    "guide_black" };

	enum class FadeType : uint8_t
	{
		Out,
		None,
		In
	};

	constexpr const char* fadeTypes[]{ "fade_out", "fade_none", "fade_in" };
}
