#include <QtGui>

#include "qSegmenter.h"

#include "ActionA.h"

// Default constructor:
//	- pass the Qt resource path to the info.json file (from <yourPluginName>.qrc file) 
//  - constructor should mainly be used to initialize actions and other members
qSegmenter::qSegmenter( QObject *parent )
	: QObject( parent )
	, ccStdPluginInterface( ":/CC/plugin/qSegmenter/info.json" )
	, m_action( nullptr )
{
}

// This method should enable or disable your plugin actions
// depending on the currently selected entities ('selectedEntities').
void qSegmenter::onNewSelection( const ccHObject::Container &selectedEntities )
{
	if ( m_action == nullptr )
	{
		return;
	}
	
	// If you need to check for a specific type of object, you can use the methods
	// in ccHObjectCaster.h or loop and check the objects' classIDs like this:
	//
	//	for ( ccHObject *object : selectedEntities )
	//	{
	//		if ( object->getClassID() == CC_TYPES::VIEWPORT_2D_OBJECT )
	//		{
	//			// ... do something with the viewports
	//		}
	//	}
	
	// For example - only enable our action if something is selected.
	// m_action->setEnabled( !selectedEntities.empty() ); // jasper: commented
}

// This method returns all the 'actions' your plugin can perform.
// getActions() will be called only once, when plugin is loaded.
QList<QAction *> qSegmenter::getActions()
{
	// default action (if it has not been already created, this is the moment to do it)
	if ( !m_action )
	{
		// Here we use the default plugin name, description, and icon,
		// but each action should have its own.
		m_action = new QAction( getName(), this );
		m_action->setToolTip( getDescription() );
		m_action->setIcon( getIcon() );

		// Connect appropriate signal
		connect( m_action, &QAction::triggered, this, [this]()
		{
			Example::performActionA( m_app );
		});
	}

	return { m_action };
}
