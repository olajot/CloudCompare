#include "SeedPicker.h"

#include <ccPickingHub.h>
#include <cc2DViewportObject.h>
#include <ccLog.h>
#include <cc2DLabel.h>
#include <ccColorTypes.h>
#include <ccPointCloud.h>
#include <ccOctree.h>
#include <queue>
#include <vector>
#include <cmath>

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
    if (!m_app)
        return;
    
    if (m_app->pickingHub())
        m_app->pickingHub()->removeListener(this);

    // Remove our markers from the DB tree
    if (m_posMarkerCloud)
    {
        m_app->removeFromDB(m_posMarkerCloud);
        m_posMarkerCloud = nullptr;
    }
    if (m_negMarkerCloud)
    {
        m_app->removeFromDB(m_negMarkerCloud);
        m_negMarkerCloud = nullptr;
    }

    m_app->refreshAll();
    m_app->dispToConsole("[Segmenter] Picking mode deactivated and markers cleared.", ccMainAppInterface::STD_CONSOLE_MESSAGE);

	// clean the segmented preview
    if (m_previewCloud)
    {
        m_app->removeFromDB(m_previewCloud);
        m_previewCloud = nullptr;
    }

    m_app->refreshAll();
    m_app->dispToConsole("[Segmenter] Picking mode deactivated and markers cleared.", ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

void SeedPicker::onItemPicked(const PickedItem& pi) //ON ITEM PICKED 
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
	
	unsigned pointIndex  = pi.itemIndex;
    const CCVector3& pointCoords = pi.P3D;
    
 

    if (m_isPositive)
    {
        m_positiveSeeds.push_back(pointIndex);
        m_app->dispToConsole(QString("[Segmenter] POSITIVE Seed added! Index: %1").arg(pointIndex));

        if (!m_posMarkerCloud)
        {
            m_posMarkerCloud = new ccPointCloud("Positive Seeds");
            m_posMarkerCloud->setPointSize(8);
            m_posMarkerCloud->resizeTheRGBTable();
            m_posMarkerCloud->showColors(true);
            
            // --- NEW: Add to the main DB so it shows in the sidebar! ---
            m_targetCloud->addChild(m_posMarkerCloud);
            m_app->addToDB(m_posMarkerCloud, false, true, false, false);
        }

        m_posMarkerCloud->addPoint(pointCoords);
        m_posMarkerCloud->addColor(ccColor::Rgb(190, 242, 58));
    }
    else
    {
        // ... [Do the exact same m_app->addToDB logic for m_negMarkerCloud] ...
        m_negativeSeeds.push_back(pointIndex);
        m_app->dispToConsole(QString("[Segmenter] NEGATIVE Seed added! Index: %1").arg(pointIndex));

        if (!m_negMarkerCloud)
        {
            m_negMarkerCloud = new ccPointCloud("Negative Seeds");
            m_negMarkerCloud->setPointSize(8);
            m_negMarkerCloud->resizeTheRGBTable();
            m_negMarkerCloud->showColors(true);
            
            m_targetCloud->addChild(m_negMarkerCloud);
            m_app->addToDB(m_negMarkerCloud, false, true, false, false);
        }

        m_negMarkerCloud->addPoint(pointCoords);
        m_negMarkerCloud->addColor(ccColor::Rgb(242, 19, 135));

    }

    m_app->refreshAll(); 

    // --- NEW: Run the region growing immediately after a click! ---
    runRegionGrowing(m_searchRadius, m_tauThreshold);
}

void SeedPicker::runRegionGrowing(double searchRadius, double tauThreshold)
{
    // 1. Safety Checks
    if (!m_targetCloud || m_positiveSeeds.empty())
    {
        m_app->dispToConsole("[Segmenter] Error: No cloud loaded or no positive seeds selected.", ccMainAppInterface::WRN_CONSOLE_MESSAGE);
        return;
    }

    // 2. Setup/Verify the Octree
    ccOctree::Shared octree = m_targetCloud->getOctree();
    if (!octree)
    {
        m_app->dispToConsole("[Segmenter] Building Octree...", ccMainAppInterface::STD_CONSOLE_MESSAGE);
        m_targetCloud->computeOctree();
        octree = m_targetCloud->getOctree();
        if (!octree) return;
    }

    m_app->dispToConsole("[Segmenter] Starting region growing...", ccMainAppInterface::STD_CONSOLE_MESSAGE);

    // 3. Initialize Tracking Variables
    m_segmentedIndices.clear();
    std::vector<bool> visited(m_targetCloud->size(), false);
    std::queue<unsigned int> pointQueue;

    // Load initial seeds into the queue
    for (unsigned int seedIdx : m_positiveSeeds)
    {
        if (seedIdx < m_targetCloud->size())
        {
            pointQueue.push(seedIdx);
            visited[seedIdx] = true;
            m_segmentedIndices.push_back(seedIdx); // Seeds are automatically part of the segment
        }
    }

   // Determine optimized octree extraction level
    unsigned char searchLevel = octree->findBestLevelForAGivenNeighbourhoodSizeExtraction(static_cast<PointCoordinateType>(searchRadius));

    //Lock the base color to the first positive seed 
    const ccColor::Rgba baseColor = m_targetCloud->getPointColor(m_positiveSeeds[0]);

    // 4. The Breadth-First Search (BFS) Loop
    while (!pointQueue.empty())
    {
        unsigned int currentIdx = pointQueue.front();
        pointQueue.pop();

        const CCVector3* p_r = m_targetCloud->getPoint(currentIdx);

        // Extract nearby points
        CCCoreLib::DgmOctree::NeighboursSet neighbors;
        octree->getPointsInSphericalNeighbourhood(*p_r, static_cast<PointCoordinateType>(searchRadius), neighbors, searchLevel);

        for (const CCCoreLib::DgmOctree::PointDescriptor& neighbor : neighbors)
        {
            unsigned int neighborIdx = neighbor.pointIndex;

            if (visited[neighborIdx])
                continue;

            // Get the color of the neighbor candidate
            const ccColor::Rgba color_n = m_targetCloud->getPointColor(neighborIdx);
            
            // integer math comparing against the ORIGINAL seed ---
            int dr = static_cast<int>(baseColor.r) - color_n.r;
            int dg = static_cast<int>(baseColor.g) - color_n.g;
            int db = static_cast<int>(baseColor.b) - color_n.b;
            
            double colorDist = std::sqrt(dr * dr + dg * dg + db * db);

            // If it meets the threshold, absorb it into the segment
            if (colorDist < tauThreshold)
            {
                visited[neighborIdx] = true;
                pointQueue.push(neighborIdx);
                m_segmentedIndices.push_back(neighborIdx);
            }
        }
    }

    // visual feedback temporary for tests
	//TODO: REmove this and replace with a more efficient way to show the segmented points (e.g. a scalar field or a dedicated color array)
    //  repaint the segmented points neon pink
    for (unsigned int idx : m_segmentedIndices)
    {
        m_targetCloud->setPointColor(idx, ccColor::magenta);
    }

    m_targetCloud->showColors(true);
    m_app->refreshAll(); // Force CloudCompare viewport update

    QString successMsg = QString("[Segmenter] Growth finished. Segmented points: %1").arg(m_segmentedIndices.size());
    m_app->dispToConsole(successMsg, ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

