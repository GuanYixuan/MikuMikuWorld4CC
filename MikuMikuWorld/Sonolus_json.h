#pragma once

#include <string>
#include <unordered_map>

#include "Score.h"
using namespace MikuMikuWorld;

// Namespace containing Sonolus json parsing functionality
namespace Sonolus_json {
	/**
	 * @brief Load given Sonolus .json file into editor
	 * @param `file_name` Full path to the given .json file
	 * @return `Score` - Converted score
	 */
	Score load_file(const std::string& file_name);

	/**
	 * @brief Upper-layer note types extracted from entity types
	 * @warning The values of these enumerate constants are exploited and therefore cannot be modified arbitrarily
	 */
	enum class Note_category {
		init,
		timing,
		single,
		slide_start,
		slide_tick,
		slide_end,
		connector,
		guide_slide,
		other
	};

	/**
	 * @brief An enum class wrapped with convenient methods to repersent entity types in the .json file
	 * @warning The values of these enumerate constants are exploited and therefore cannot be modified arbitrarily
	 * @note Enumerate constants in this class are closely related to .json file format (Up to 23/10/03)
	 */
	class Entity_type {
	public:
		enum __Inner_type : char {
			// Initialization-related
			Initialization,
			InputManager,
			Stage,

			// Time scale and BPM changes
			TimeScaleGroup,
			TimeScaleChange,
			BPMChange,

			// Single note
			NormalTap = 0x10,
			CriticalTap,
			NormalTrace,
			CriticalTrace,
			NormalFlick,
			CriticalFlick,
			NormalTraceFlick,
			CriticalTraceFlick,

			// Slide start
			NormalSlideStart = 0x20,
			CriticalSlideStart,
			NormalTraceSlideStart,
			CriticalTraceSlideStart,
			HiddenSlideStart, // Used as the start/end of guide slides, can also appear in normal slides

			// Slide tick
			NormalSlideTick = 0x30,
			CriticalSlideTick,
			NormalAttachedSlideTick, // Ticks with a diamond but doesn't control the curve
			CriticalAttachedSlideTick,
			HiddenSlideTick, // Ticks without a diamond but still controls the curve, also appear in guide slides
			IgnoredSlideTick, // Ticks automatically added to slides per half-beat

			// Slide end
			NormalSlideEnd = 0x40,
			CriticalSlideEnd,
			NormalTraceSlideEnd,
			CriticalTraceSlideEnd,
			NormalSlideEndFlick,
			CriticalSlideEndFlick,

			// Slide connector (slide bar)
			NormalSlideConnector = 0x50,
			CriticalSlideConnector,

			// Guide slide
			Guide = 0x60,

			// Others
			SimLine = 0x70, // The link between two synchronous notes
			DamageNote
		} __value;
		/**
		 * @brief Construct from string
		 * @throw `std::invalid_argument` - `str` isn't a valid type
		 */
		explicit Entity_type(const std::string& str);
		// Allow constructions like `Entity_type foo(Entity_type::BPMChange)`
		constexpr Entity_type(__Inner_type val) noexcept : __value(val) {}
		// Allow switch and comparisons
		constexpr operator __Inner_type() const noexcept { return __value; }
		// Prevent `if(entity_type)` usage
		explicit operator bool() const = delete;

		// Returns whether this entity is a note
		bool is_note() const noexcept;
		// Returns the category this entity belongs to
		Note_category get_category() const noexcept;
		/**
		 * @brief Returns whether this *note* is a critical note
		 * @throw `std::invalid_argument` - Current entity isn't a note (e.g. BPMChange or something else)
		 * @note "YellowDummySlide" is considered the critical version of DummySlide
		 */
		bool critical() const;
		/**
		 * @brief Returns whether this *note* is a friction note
		 * @throw `std::invalid_argument` - Current entity isn't a note (e.g. BPMChange or something else)
		 */
		bool friction() const;

	private:
		// Mapping from strings presented in .json file to enum objects
		static const std::unordered_map<std::string, __Inner_type> map_from_str;
	};
}
