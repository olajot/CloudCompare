
#include "ActionA.h"

#include "SeedPicker.h"
#include "SegmenterDlg.h"
#include "ccMainAppInterface.h"
#include "ccPointCloud.h" // Needed for Octree calculation

#include <QMainWindow>

namespace Example
{
	// This is an example of an action's method called when the corresponding action
	// is triggered (i.e. the corresponding icon or menu entry is clicked in CC's
	// main interface). You can access most of CC's components (database,
	// 3D views, console, etc.) via the 'appInterface' variable.

	// We keep a static pointer for now just so it persists in the background.
	// (Later, this should be a member variable of your qSegmenter class or UI dialog).
	static SeedPicker* g_seedPicker = nullptr;
	static SegmenterDlg* g_dialog     = nullptr;

	void performActionA(ccMainAppInterface* appInterface)
	{
		if (appInterface == nullptr)
			return;

		// --- SILENT OCTREE CALCULATION ---
		// Let's grab the currently selected point cloud and build the octree
		// before the Picking Hub has a chance to complain.
		const ccHObject::Container& selected = appInterface->getSelectedEntities();
		for (ccHObject* obj : selected)
		{
			if (obj->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* cloud = static_cast<ccPointCloud*>(obj);
				if (!cloud->getOctree())
				{
					appInterface->dispToConsole("[Segmenter] Computing Octree for faster picking...", ccMainAppInterface::STD_CONSOLE_MESSAGE);
					// This computes the octree. We pass the main window to show a progress bar if it takes a while!
					//cloud->computeOctree(appInterface->getMainWindow());
					cloud->computeOctree();
				}
				break; // Just grab the first point cloud we find
			}
		}

		// --- INIT UI ---
		if (g_dialog == nullptr)
		{
			// Pass the main window as the parent so the dialog floats correctly over CloudCompare
			g_dialog = new SegmenterDlg(appInterface->getMainWindow());

			// destroy the dialog object when the window is closed
			g_dialog->setAttribute(Qt::WA_DeleteOnClose);

			// Listen for when the dialog is destroyed to clean up our pointers and stop picking
			QObject::connect(g_dialog, &QObject::destroyed, [appInterface]()
			                 {
                g_dialog = nullptr; // Reset the pointer
                
                if (g_seedPicker) {
                    g_seedPicker->stopListening();
                    delete g_seedPicker;
                    g_seedPicker = nullptr;
                }
                appInterface->dispToConsole("[Segmenter] Dialog closed. Picking stopped.", ccMainAppInterface::STD_CONSOLE_MESSAGE); });
		}

		// 2. Initialize the Seed Picker if it doesn't exist
		if (g_seedPicker == nullptr)
		{
			g_seedPicker = new SeedPicker(appInterface);
			g_seedPicker->startListening();
		}

		// --- WIRE UI TO PICKER ---
		// Ensure picker starts with the correct mode based on the UI's default
		g_seedPicker->setPositiveMode(g_dialog->isAddingPositiveSeeds());

		// Tell the picker to update whenever the UI state changes (e.g. someone clicked a radio button)
		QObject::connect(g_dialog, &SegmenterDlg::stateChanged, []()
		                 {
            if (g_seedPicker && g_dialog) {
                g_seedPicker->setPositiveMode(g_dialog->isAddingPositiveSeeds());
            } });

		// --- SHOW UI ---
		g_dialog->show();
		g_dialog->raise();          // Bring window to the front
		g_dialog->activateWindow(); // Give it focus

		appInterface->dispToConsole("[Segmenter] UI Opened and Picking Started!", ccMainAppInterface::STD_CONSOLE_MESSAGE);
	}
} 