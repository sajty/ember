#include <SDL.h>

#include <sigc++/sigc++.h>

// #include <Eris/View.h>

#include "components/ogre/MovementController.h"
#include "components/ogre/Avatar.h"
#include "components/ogre/camera/MainCamera.h"
#include "components/ogre/EmberOgre.h"

#include "components/ogre/EmberEntityFactory.h"

#include "components/ogre/EmberEntity.h"

#include "components/ogre/environment/Environment.h"

#include "components/ogre/MousePicker.h"

#include "components/ogre/MotionManager.h"
#include "components/ogre/GUIManager.h"
#include "components/ogre/terrain/TerrainInfo.h"
#include "components/ogre/terrain/TerrainManager.h"
#include "components/ogre/terrain/TerrainArea.h"

#include "components/ogre/model/Model.h"
#include "components/ogre/model/SubModel.h"

#include "components/ogre/lod/LodDefinition.h"
#include "components/ogre/lod/LodDefinitionManager.h"
#include "components/ogre/lod/LodManager.h"

#include "components/ogre/widgets/Widget.h"

#include "components/ogre/Convert.h"

#include "components/ogre/widgets/MovableObjectRenderer.h"
#include "components/ogre/widgets/OgreEntityRenderer.h"
#include "components/ogre/widgets/ModelRenderer.h"
#include "components/ogre/widgets/ListHolder.h"
#include "components/ogre/widgets/Vector3Adapter.h"
#include "components/ogre/widgets/QuaternionAdapter.h"
#include "components/ogre/widgets/EntityEditor.h"
#include "components/ogre/widgets/StackableContainer.h"
#include "components/ogre/widgets/ConsoleAdapter.h"
#include "components/ogre/widgets/ColouredListItem.h"
#include "components/ogre/widgets/MeshInfoProvider.h"
#include "components/ogre/widgets/adapters/atlas/MapAdapter.h"


#include "components/ogre/widgets/IconBar.h"
#include "components/ogre/widgets/IconBase.h"
//#include "components/ogre/widgets/MaterialPicker.h"

#include "components/ogre/model/Model.h"
#include "components/ogre/model/ModelDefinition.h"
#include "components/ogre/model/ModelDefinitionManager.h"
#include "components/ogre/model/ModelDefinitionAtlasComposer.h"

#include "components/ogre/IWorldPickListener.h"
#include "components/ogre/EntityWorldPickListener.h"

#include "components/ogre/authoring/EntityMoveManager.h"

#include "components/ogre/OgreInfo.h"

#include "components/ogre/SimpleRenderContext.h"


#include "components/ogre/widgets/icons/Icon.h"
#include "components/ogre/widgets/icons/IconManager.h"

#include "components/ogre/widgets/EntityIconSlot.h"
#include "components/ogre/widgets/EntityIcon.h"
#include "components/ogre/widgets/EntityIconManager.h"
#include "components/ogre/widgets/EntityIconDragDropTarget.h"
