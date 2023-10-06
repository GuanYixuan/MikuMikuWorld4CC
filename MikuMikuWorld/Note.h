#pragma once
#include "NoteTypes.h"
#include <vector>
#include <string>

namespace MikuMikuWorld
{
	constexpr const char* SE_PERFECT			= "perfect";
	constexpr const char* SE_FLICK				= "flick";
	constexpr const char* SE_CONNECT			= "connect";
	constexpr const char* SE_TICK				= "tick";
	constexpr const char* SE_CRITICAL_TAP		= "critical_tap";
	constexpr const char* SE_CRITICAL_FLICK		= "critical_flick";
	constexpr const char* SE_CRITICAL_CONNECT	= "critical_connect";
	constexpr const char* SE_CRITICAL_TICK		= "critical_tick";
	constexpr const char* SE_FRICTION			= "friction";
	constexpr const char* SE_CRITICAL_FRICTION	= "critical_friction";

	constexpr const char* SE_NAMES[] =
	{
		SE_PERFECT,
		SE_FLICK,
		SE_CONNECT,
		SE_TICK,
		SE_CRITICAL_TAP,
		SE_CRITICAL_FLICK,
		SE_CRITICAL_CONNECT,
		SE_CRITICAL_TICK,
		SE_FRICTION,
		SE_CRITICAL_FRICTION
	};

	constexpr float flickArrowWidths[] =
	{
		0.95f, 1.25f, 1.8f, 2.3f, 2.6f, 3.2f
	};

	constexpr float flickArrowHeights[] =
	{
		1, 1.05f, 1.2f, 1.4f, 1.5f, 1.6f
	};

	enum class ZIndex : int32_t
	{
		HoldLine,
		Note,
		FlickArrow
	};

	struct NoteTextures
	{
		int notes;
		int holdPath;
		int touchLine;
	};

	extern NoteTextures noteTextures;

	struct Score;

	extern int nextID;

	class Note final
	{
	private:
		NoteType type;

	public:
		int ID;
		int parentID;
		int tick;
		int lane;
		int width;
		bool critical;
		bool friction;
		FlickType flick;

		Note(NoteType _type);
		Note();

		inline constexpr NoteType getType() const { return type; }

		bool isFlick() const;
		bool hasEase() const;
	};

	struct HoldStep
	{
		int ID;
		HoldStepType type;
		EaseType ease;
	};

	class HoldNote final
	{
	public:
		HoldStep start;
		std::vector<HoldStep> steps;
		int end;

		HoldNoteType startType{};
		HoldNoteType endType{};

		constexpr bool isGuide() const
		{ 
			return startType == HoldNoteType::Guide || endType == HoldNoteType::Guide;
		}
	};

	void resetNextID();

	void cycleFlick(Note& note);
	void cycleStepEase(HoldStep& note);
	void cycleStepType(HoldStep& note);
	void sortHoldSteps(const Score& score, HoldNote& note);
	int findHoldStep(const HoldNote& note, int stepID);

	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int getFrictionSpriteIndex(const Note& note);
	std::string_view getNoteSE(const Note& note, const Score& score);
}
