#include "SeedPicker.h"

#include <ccPickingHub.h>
#include <cc2DViewportObject.h>
#include <ccLog.h>

SeedPicker::SeedPicker(ccMainAppInterface* app)
    : m_app(app)
    , m_targetCloud(nullptr)
{
}

SeedPicker::~SeedPicker()
{
	stopListening();
}

void SeedPicker::startListening()
{
	if (!m_app || !m_app->pickingHub())
		return;

	// Register this class to the Hub.
	// exclusive = false (allows other plugins to listen, usually fine)
	// autoStartPicking = true (forces the viewport into picking mode)
	// mode = POINT_PICKING (we only care about points, not triangles)
	bool success = m_app->pickingHub()->addListener(
	    this,
	    false,
	    true,
	    ccGLWindowInterface::POINT_PICKING);

	if (success)
		m_app->dispToConsole("[Segmenter] Picking mode activated. Click points!", ccMainAppInterface::STD_CONSOLE_MESSAGE);
	else
		m_app->dispToConsole("[Segmenter] Error: Could not start picking.", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
}

void SeedPicker::stopListening()
{
	if (!m_app || !m_app->pickingHub())
	return;
	
	m_app->pickingHub()->removeListener(this);
	m_app->dispToConsole("[Segmenter] Picking mode deactivated.", ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

void SeedPicker::onItemPicked(const PickedItem& pi)
{
	// verify user actually clicked an entity
	if (!pi.entity)
	return;
	
	// verify it's a PC
	if (!pi.entity->isA(CC_TYPES::POINT_CLOUD))
	{
		m_app->dispToConsole("[Segmenter] Please click on a point cloud.", ccMainAppInterface::WRN_CONSOLE_MESSAGE);
		return;
	}
	
	ccPointCloud* clickedCloud = static_cast<ccPointCloud*>(pi.entity);
	
	// ensure we are only clicking on ONE cloud during the session
	if (!m_targetCloud)
	{
		m_targetCloud = clickedCloud;
	}
	else if (m_targetCloud != clickedCloud)
	{
		m_app->dispToConsole("[Segmenter] Warning: Clicked a different cloud. Ignored.", ccMainAppInterface::WRN_CONSOLE_MESSAGE);
		return;
	}
	
	unsigned         pointIndex  = pi.itemIndex;
	const CCVector3& pointCoords = pi.P3D;
	
	//route the point to the correct list
	if (m_isPositive)
	{
		m_positiveSeeds.push_back(pointIndex); // push back means append to the end
		QString msg = QString("[Segmenter] POSITIVE Seed added! Index: %1").arg(static_cast<int>(pointIndex));
		m_app->dispToConsole(msg, ccMainAppInterface::STD_CONSOLE_MESSAGE);
	}
	else
	{
		m_negativeSeeds.push_back(pointIndex);
		QString msg = QString("[Segmenter] NEGATIVE Seed added! Index: %1").arg(static_cast<int>(pointIndex));
		m_app->dispToConsole(msg, ccMainAppInterface::STD_CONSOLE_MESSAGE);
	}
		
		m_app->refreshAll(); // force redraw
}

