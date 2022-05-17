#pragma once
#include "include/pugixml.hpp"
#include <iostream>
#include <vector>

enum class LiveSplitFileStatus
{
	OK,
	NO_SEGMENTS,
	WRONG_FILE,
};

struct SegmentInfo
{
	const pugi::char_t* name;
	const pugi::char_t* realTime;
	const pugi::char_t* gameTime;
	const pugi::char_t* bestRealTime;
	const pugi::char_t* bestGameTime;
};

struct SplitsInfo
{
	const pugi::char_t* gameName = nullptr;
	const pugi::char_t* categoryName = nullptr;
	int attemptCount = 0;

	std::vector<SegmentInfo*> segments;
};

class LiveSplitReader
{
public:
	LiveSplitReader(const wchar_t* path);
	~LiveSplitReader();

	pugi::xml_document m_doc;
	pugi::xml_parse_result m_result;
	pugi::xml_node m_rootNode;

	LiveSplitFileStatus m_livesplitResult;
	SplitsInfo m_splits;
};

