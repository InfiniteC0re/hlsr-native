#include "LiveSplitReader.h"

LiveSplitReader::LiveSplitReader(const wchar_t* path)
{
	m_result = m_doc.load_file(path);
	m_livesplitResult = LiveSplitFileStatus::WRONG_FILE;

	if (m_result.status == pugi::xml_parse_status::status_ok)
	{
		m_rootNode = m_doc.child(L"Run");

		if (m_rootNode)
		{
			m_livesplitResult = LiveSplitFileStatus::OK;
			m_splits.gameName = m_rootNode.child_value(L"GameName");
			m_splits.attemptCount = m_rootNode.child(L"AttemptCount").text().as_int();
			m_splits.categoryName = m_rootNode.child_value(L"CategoryName");

			pugi::xml_node segmentsRoot = m_rootNode.child(L"Segments");

			if (segmentsRoot)
			{
				pugi::xpath_node_set segments = segmentsRoot.select_nodes(L"Segment");

				for (int i = 0; i < segments.size(); i++)
				{
					pugi::xml_node segmentNode = segments[i].node();
					pugi::xml_node splitTimeNode = segmentNode.select_node(L"SplitTimes").node().select_node(L"SplitTime[@name='Personal Best']").node();
					pugi::xml_node bestSegmentTimeNode = segmentNode.select_node(L"BestSegmentTime").node();
					
					SegmentInfo* pSegment = new SegmentInfo();
					pSegment->name = segmentNode.child_value(L"Name");
					pSegment->realTime = splitTimeNode.child_value(L"RealTime");
					pSegment->gameTime = splitTimeNode.child_value(L"GameTime");
					pSegment->bestRealTime = bestSegmentTimeNode.child_value(L"RealTime");
					pSegment->bestGameTime = bestSegmentTimeNode.child_value(L"GameTime");

					m_splits.segments.push_back(pSegment);
				}
			}
			else
			{
				m_livesplitResult = LiveSplitFileStatus::NO_SEGMENTS;
			}
		}
	}
}

LiveSplitReader::~LiveSplitReader()
{
	for (int i = 0; i < m_splits.segments.size(); i++)
		delete m_splits.segments[i];

	m_doc.reset();
}
