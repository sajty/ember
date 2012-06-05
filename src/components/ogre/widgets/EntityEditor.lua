--Allows the editing of entities
EntityEditor = {
	
	adapters = {
		map = {
			name = "Map",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createMapAdapter(wrapper.container, self.instance.entity:getId(), element)
				if wrapper.adapter == nil then
					return nil
				end
				
				local attributeNames = wrapper.adapter:getAttributeNames()
				for i = 0, attributeNames:size() - 1 do
					local name = attributeNames[i]
					local childElement = wrapper.adapter:valueOfAttr(name)
					local adapterWrapper = self:createAdapter(name, childElement)
					if adapterWrapper then
						if adapterWrapper.adapter then
							self:addNamedAdapterContainer(name, adapterWrapper.adapter, adapterWrapper.container, wrapper.container, adapterWrapper.prototype)
							wrapper.adapter:addAttributeAdapter(name, adapterWrapper.adapter, adapterWrapper.outercontainer)
						end
					end
				end
				
				if prototype.readonly == nil then
					local newElementWrapper = self.adapters.map.createNewElementWidget(self, wrapper.adapter, wrapper.container, element)
					wrapper.container:addChildWindow(newElementWrapper.container)
				end
				self:createStackableContainer(wrapper.container):repositionWindows()
				return wrapper
			end,
			createNewElement = function(self)
				return self.instance.helper:createMapElement()
			end,
			createNewElementWidget = function(self, mapAdapter, outercontainer, outerElement)
				local wrapper = {}
				wrapper.adapter = mapAdapter
				wrapper.outercontainer = outercontainer
				wrapper.container = guiManager:createWindow("DefaultWindow")
				self.factory:loadLayoutIntoContainer(wrapper.container, "newNamedElement", "adapters/atlas/MapAdapterNewElement.layout")
				wrapper.button = CEGUI.toPushButton(windowManager:getWindow(self.factory:getCurrentPrefix().. "NewElementButton"))
				wrapper.container:setHeight(CEGUI.UDim(0, 25))
				wrapper.typeCombobox = CEGUI.toCombobox(windowManager:getWindow(self.factory:getCurrentPrefix().. "ElementType"))
				wrapper.newAdapters = self:fillNewElementCombobox(wrapper.typeCombobox, "", outerElement)
				wrapper.nameEditbox = CEGUI.toCombobox(windowManager:getWindow(self.factory:getCurrentPrefix().. "ElementName"))
				wrapper.nameChanged = function(args)
					--check that the name doesn't already exists in the map adapter
					if mapAdapter:hasAdapter(wrapper.nameEditbox:getText()) then
						wrapper.button:setEnabled(false)
					else
						wrapper.newAdapters = self:fillNewElementCombobox(wrapper.typeCombobox, wrapper.nameEditbox:getText(), outerElement)
						wrapper.buttonEnableChecker(args)
					end
					return true
				end
				wrapper.nameChanged = wrapper.nameEditbox:getEditbox():subscribeEvent("TextChanged", wrapper.nameChanged)
				
				for index,value in pairs(self.prototypes) do
					if value.shouldAddSuggestion then
						if value.shouldAddSuggestion(outerElement) then
							local item = Ember.OgreView.Gui.ColouredListItem:new(index)
							wrapper.nameEditbox:addItem(item)
						end
					end
				end
				
				wrapper.buttonEnableChecker = function(args)
					if wrapper.typeCombobox:getSelectedItem() ~= nil and wrapper.nameEditbox:getText() ~= "" then
						wrapper.button:setEnabled(true)
					else
						wrapper.button:setEnabled(false)
					end
					return true
				end
				wrapper.typeCombobox:subscribeEvent("ListSelectionAccepted", wrapper.buttonEnableChecker)
				
				wrapper.buttonPressed = function(args)
					local name = wrapper.nameEditbox:getText()
					local newAdapter = wrapper.newAdapters[wrapper.typeCombobox:getSelectedItem():getID()]
					if newAdapter.createNewElement then
						local element = newAdapter.createNewElement(self)
						local adapterWrapper = newAdapter.createAdapter(self, element, self:getPrototype(name, element))
						
						self.instance.addNewElement(self, element)
						
						if adapterWrapper then
							local newPrototype = {}
							wrapper.adapter:addAttributeAdapter(name, adapterWrapper.adapter, adapterWrapper.outercontainer)
							self:addNamedAdapterContainer(name, adapterWrapper.adapter, adapterWrapper.container, wrapper.outercontainer, newPrototype)
							--by adding the window again we make sure that it's at the bottom of the child window list
							wrapper.outercontainer:addChildWindow(wrapper.container)
							wrapper.nameEditbox:setText("")
						end
					end
					return true
				end
				wrapper.buttonSubscriber = wrapper.button:subscribeEvent("Clicked", wrapper.buttonPressed)
			
				wrapper.buttonEnableChecker(nil)

				return wrapper
			end
		},
		list = {
			name = "List",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createListAdapter(wrapper.container, self.instance.entity:getId(), element)
				if wrapper.adapter == nil then
					return nil
				end
				for i = 0, wrapper.adapter:getSize() - 1 do
					local childElement = wrapper.adapter:valueOfAttr(i)
					local childPrototype = self:getPrototype("", childElement)
					--if the prototype for the list have it marked as nodelete, mark the childelements too
					if prototype.readonly then
						childPrototype.readonly = true
						childPrototype.nodelete = true
					end
					local adapterWrapper = self:createAdapterFromPrototype(childElement, childPrototype)
					if adapterWrapper then
						self:addUnNamedAdapterContainer(adapterWrapper.adapter, adapterWrapper.container, wrapper.container, adapterWrapper.prototype)
						wrapper.adapter:addAttributeAdapter(adapterWrapper.adapter, adapterWrapper.outercontainer)
					end
				end	
				
				if prototype.readonly == nil then
					local newElementWrapper = self.adapters.list.createNewElementWidget(self, wrapper.adapter, wrapper.container)
					wrapper.container:addChildWindow(newElementWrapper.container)
				end
				self:createStackableContainer(wrapper.container):repositionWindows()
				
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createListElement()
			end,
			createNewElementWidget = function(self, listAdapter, outercontainer, prototype)
				local wrapper = {}
				wrapper.adapter = listAdapter
				wrapper.outercontainer = outercontainer
				wrapper.container = guiManager:createWindow("DefaultWindow")
				self.factory:loadLayoutIntoContainer(wrapper.container, "newUnnamedElement", "adapters/atlas/ListAdapterNewElement.layout")
				wrapper.button = CEGUI.toPushButton(windowManager:getWindow(self.factory:getCurrentPrefix().. "NewElementButton"))
				wrapper.container:setHeight(CEGUI.UDim(0, 25))
				wrapper.typeCombobox = CEGUI.toCombobox(windowManager:getWindow(self.factory:getCurrentPrefix().. "ElementType"))
				wrapper.newAdapters = self:fillNewElementCombobox(wrapper.typeCombobox, "")
				wrapper.buttonPressed = function(args)
					local newAdapter = wrapper.newAdapters[wrapper.typeCombobox:getSelectedItem():getID()]
					if newAdapter.createNewElement then
						local element = newAdapter.createNewElement(self)
						local adapterWrapper = newAdapter.createAdapter(self, element, self:getPrototype("", element))
						
						--store a reference to the element so it isn't garbage collected
						self.instance.addNewElement(self, element)
						
						if adapterWrapper then
							wrapper.adapter:addAttributeAdapter(adapterWrapper.adapter, adapterWrapper.outercontainer)
							local newPrototype = {}
							self:addUnNamedAdapterContainer(adapterWrapper.adapter, adapterWrapper.container, wrapper.outercontainer, newPrototype)
							--by adding the window again we make sure that it's at the bottom of the child window list
							wrapper.outercontainer:addChildWindow(wrapper.container)
						end
					end
				end
				wrapper.button:subscribeEvent("Clicked", wrapper.buttonPressed)
			
				wrapper.buttonEnableChecker = function(args)
					if wrapper.typeCombobox:getSelectedItem() then
						wrapper.button:setEnabled(true)
					else
						wrapper.button:setEnabled(false)
					end
					return true
				end
				wrapper.typeCombobox:subscribeEvent("ListSelectionAccepted", wrapper.buttonEnableChecker)
				
				wrapper.buttonEnableChecker(nil)
			
				return wrapper
			end
		},
		static = {
			name = "Static",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createStaticAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end
		},
		size = {
			name = "Size",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createSizeAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createListElement()
			end
		},
		position = {
			name = "Position",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createPositionAdapter(wrapper.container, self.instance.entity:getId(), element)
				wrapper.moveButtonPressed = function()
					guiManager:EmitEntityAction("move", self.instance.entity)
				end
				wrapper.moveButtonPressedListener = createConnector(wrapper.adapter.EventMoveClicked):connect(wrapper.moveButtonPressed)
				return wrapper
			end
		},
		position2d = {
			name = "Position",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createPosition2DAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createPosition2dElement()
			end
		},
		orientation = {
			name = "Orientation",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createOrientationAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end
		},
		points = {
			name = "Points",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createListAdapter(wrapper.container, self.instance.entity:getId(), element)
				if wrapper.adapter == nil then
					return nil
				end
				for i = 0, wrapper.adapter:getSize() - 1 do
					local childElement = wrapper.adapter:valueOfAttr(i)
					local childPrototype = self.adapters.position2d
					local adapterWrapper = self:createAdapterFromPrototype(childElement, childPrototype)
					if adapterWrapper then
						self:addUnNamedAdapterContainer(adapterWrapper.adapter, adapterWrapper.container, wrapper.container, adapterWrapper.prototype)
						wrapper.adapter:addAttributeAdapter(adapterWrapper.adapter, adapterWrapper.outercontainer)
					end
				end	
				
				local newElementWrapper = self.adapters.points.createNewElementWidget(self, wrapper.adapter, wrapper.container)
				wrapper.container:addChildWindow(newElementWrapper.container)
				self:createStackableContainer(wrapper.container):repositionWindows()
				
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createListElement()
			end,
			createNewElementWidget = function(self, listAdapter, outercontainer, prototype)
				local wrapper = {}
				wrapper.adapter = listAdapter
				wrapper.outercontainer = outercontainer
				wrapper.container = guiManager:createWindow("DefaultWindow")
				self.factory:loadLayoutIntoContainer(wrapper.container, "newUnnamedElement", "adapters/atlas/ListAdapterNewElement.layout")
				wrapper.container:setHeight(CEGUI.UDim(0, 25))
				wrapper.typeCombobox = CEGUI.toCombobox(windowManager:getWindow(self.factory:getCurrentPrefix().. "ElementType"))
				
				local item = Ember.OgreView.Gui.ColouredListItem:new("Point", 0)
				wrapper.typeCombobox:addItem(item)
				wrapper.typeCombobox:setHeight(CEGUI.UDim(0, 100))
				--combobox:setProperty("ReadOnly", "true")
				
				wrapper.button = CEGUI.toPushButton(windowManager:getWindow(self.factory:getCurrentPrefix().. "NewElementButton"))
				wrapper.buttonPressed = function(args)
					local newAdapter = newAdapters[wrapper.typeCombobox:getSelectedItem():getID()]
					if newAdapter.createNewElement then
						local element = newAdapter.createNewElement(self)
						local adapterWrapper = newAdapter.createAdapter(self, element, self:getPrototype("", element))
						
	--[[					local adapterWrapper = nil
						local element = nil
						
						if wrapper.typeCombobox:getSelectedItem():getID() == 0 then
							element = self.instance.helper:createPosition2dElement()
							adapterWrapper = self.adapters.position2d.createAdapter(self, element, self:getPrototype("", element))
						end]]
						
						self.instance.addNewElement(self, element)
						
						if adapterWrapper then
							local newPrototype = {}
							wrapper.adapter:addAttributeAdapter(adapterWrapper.adapter, adapterWrapper.outercontainer)
							self:addUnNamedAdapterContainer(adapterWrapper.adapter, adapterWrapper.container, wrapper.outercontainer, newPrototype)
							--by adding the window again we make sure that it's at the bottom of the child window list
							wrapper.outercontainer:addChildWindow(wrapper.container)
						end
					end
				end
				wrapper.button:subscribeEvent("Clicked", wrapper.buttonPressed)
			
				return wrapper
			end

		},
		string = {
			name = "String",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createStringAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createStringElement()
			end
		},
		number = {
			name = "Number",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createNumberAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end
		},
		float = {
			name = "Float",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createNumberAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createFloatElement()
			end
			
		},
		integer = {
			name = "Integer",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createNumberAdapter(wrapper.container, self.instance.entity:getId(), element)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createIntElement()
			end
		},
		area = {
			name = "Area",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createAreaAdapter(wrapper.container, self.instance.entity:getId(), element, self.instance.entity)
				
				wrapper.adapter:addAreaSuggestion(0, "none")
				
				--fill the area adapter with suggested areas, which we get from the terrain layer definitions
				local layerDefinitions = Ember.OgreView.Terrain.TerrainLayerDefinitionManager:getSingleton():getDefinitions()
				for index,value in layerDefinitions:ipairs() do
					if value:getAreaId() ~= 0 then
						local name = value:getName()
						--fall back to the area id if there's no name given
						if name == "" then
							name = value:getAreaId() 
						end
						wrapper.adapter:addAreaSuggestion(value:getAreaId(), name)
					end
				end 
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createMapElement()
			end
		},
		terrainmod = {
			name = "Terrain mod",
			createAdapter = function(self, element, prototype)
				local wrapper = {}
				wrapper.container = guiManager:createWindow("DefaultWindow")
				wrapper.adapter = self.factory:createTerrainModAdapter(wrapper.container, self.instance.entity:getId(), element, self.instance.entity)
				return wrapper	
			end,
			createNewElement = function(self)
				return self.instance.helper:createMapElement()
			end
		}
	}
}
EntityEditor.prototypes = 
{
	external = {
		adapter = EntityEditor.adapters.static
	},
	parents = {
		nodelete = true,
-- 		adapter = EntityEditor.adapters.static
		adapter = nil
	},
	objtype = {
		nodelete = true,
		adapter = EntityEditor.adapters.static
	},
	velocity = {
		adapter = nil
	},
	stamp = {
		adapter = nil,
		nodelete = true
	},
	name = {
		adapter = EntityEditor.adapters.string,
		nodelete = true
	},
	bbox = {
		adapter = EntityEditor.adapters.size,
		nodelete = true
	},
	pos = {
		adapter = EntityEditor.adapters.position,
		nodelete = true
	},
	orientation = {
		adapter = EntityEditor.adapters.orientation,
		nodelete = true
	},
	area = {
		adapter = EntityEditor.adapters.area,
		shouldAddSuggestion = function(ownerElement)
			return true
		end
	},
	points = {
		adapter = EntityEditor.adapters.points
	},
	style = {
		adapter = EntityEditor.adapters.string,
		suggestions = {
			"gnarly",
			"knotted",
			"weathered"
		}
	},
	terrainmod = {
		adapter = EntityEditor.adapters.terrainmod,
		shouldAddSuggestion = function(ownerElement)
			return true
		end
	}
}
EntityEditor.defaultPrototypes = 
{
	string = {
		adapter = EntityEditor.adapters.string
	},
	integer = {
		adapter = EntityEditor.adapters.integer
	},
	float = {
		adapter = EntityEditor.adapters.float
	},
	list = {
		adapter = EntityEditor.adapters.list
	},
	map = {
		adapter = EntityEditor.adapters.map
	}
}


function editEntity(id)
	local entity = emberOgre:getWorld():getEmberEntity(id)
	if entity then
		self:editEntity(entity)
	end
end

function EntityEditor:createStackableContainer(container)
	local stackableContainer = Ember.OgreView.Gui.StackableContainer:new_local(container)
	stackableContainer:setInnerContainerWindow(container)
	self.instance.stackableContainers[container:getName()] = stackableContainer
	return stackableContainer
end

function EntityEditor:clearEditing()
	if self.instance then
		
		if self.instance.entity then
			--as we're not editing anymore, hide the bounding boxes
			self.world:getAuthoringManager():hideSimpleEntityVisualization(self.instance.entity)
			self.instance.entity:setVisualize("OgreBBox", false)
		end
		
		--we want to disconnect all stackable containers before we start
		for index,value in ipairs(self.instance.stackableContainers) do
			value:disconnect()
		end
	
		if self.instance.rootMapAdapter then
			self.instance.rootMapAdapter:removeAdapters()
		end
		if self.instance.outercontainer then
			windowManager:destroyWindow(self.instance.outercontainer)
		end
		if self.instance.deleteListener then
			self.instance.deleteListener:disconnect()
		end
		if self.instance.entityChangeConnection then
			self.instance.entityChangeConnection:disconnect()
		end
		if self.instance.helper then
			self.instance.helper:removeMarker()
		end
		deleteSafe(self.instance.helper)
		self.instance = nil
	end
	self.instance = {knowledge={model={}}}
	self.instance.stackableContainers = {}
	self.instance.newElements = {}
	self.instance.addNewElement = function(self, element) 
		table.insert(self.instance.newElements, element)
	end
	
end

function EntityEditor:editEntity(entity)
	self.widget:show()

	self:clearEditing()
	
	self.instance.model = {}
	self.instance.entity = entity
	
	--show the bounding boxes by default when editing
	self.instance.entity:setVisualize("OgreBBox", false)
	self.world:getAuthoringManager():displaySimpleEntityVisualization(self.instance.entity)
	
	self.instance.deleteListener = createConnector(entity.BeingDeleted):connect(self.Entity_BeingDeleted, self)
	
	
	self:refreshChildren(entity)
	self:refreshModelInfo(entity)
	
	
	self.instance.entityChangeConnection = createConnector(entity.Changed):connect(self.Entity_Changed, self)
	self.instance.outercontainer = guiManager:createWindow("DefaultWindow")
	local adapter = self.factory:createMapAdapter(self.instance.outercontainer, self.instance.entity:getId(), self.instance.entity)
	self.instance.rootMapAdapter = adapter
	self.instance.helper = Ember.OgreView.Gui.EntityEditor:new(self.world, entity, self.instance.rootMapAdapter)
	self.attributesContainer:addChildWindow(self.instance.outercontainer)
	
	local attributeNames = self.instance.rootMapAdapter:getAttributeNames()
	for i = 0, attributeNames:size() - 1 do
		local name = attributeNames[i]
		local element = self.instance.rootMapAdapter:valueOfAttr(name)
		local prototype = self:getPrototype(name, element)
		--there's currently no way to delete from the root attributes, so we'll just disallow that
		prototype.nodelete = true
		local adapterWrapper = self:createAdapterFromPrototype(element, prototype)
		if adapterWrapper then
			self.instance.rootMapAdapter:addAttributeAdapter(name, adapterWrapper.adapter, adapterWrapper.outercontainer)
			self:addNamedAdapterContainer(name, adapterWrapper.adapter, adapterWrapper.container, self.instance.outercontainer, adapterWrapper.prototype)
		end
	end
	self.instance.model.newAdapter = self.adapters.map.createNewElementWidget(self, adapter, self.instance.outercontainer)
	self.instance.outercontainer:addChildWindow(self.instance.model.newAdapter.container)
	self:createStackableContainer(self.instance.outercontainer):repositionWindows()

	self.infoWindow:setText('Id: ' .. entity:getId() .. ' Name: ' .. entity:getName())

	self.knowledgelistbox:resetList()
	self.goallistbox:resetList()
	
end

function EntityEditor:createAdapter(attributeName, element)
	local prototype = self:getPrototype(attributeName, element)
	return self:createAdapterFromPrototype(element, prototype)
end

function EntityEditor:createAdapterFromPrototype(element, prototype)
	local adapterWrapper = nil
	if prototype.adapter then
		adapterWrapper = prototype.adapter.createAdapter(self, element, prototype)
		if adapterWrapper then
			if prototype.suggestions then
				for index,value in ipairs(prototype.suggestions) do
					adapterWrapper.adapter:addSuggestion(value)
				end
			end
			adapterWrapper.prototype = prototype
		end
	end
	return adapterWrapper
end

function EntityEditor:getPrototype(attributeName, element)
	local prototype = {}
	if self.prototypes[attributeName] then
		 prototype = self.prototypes[attributeName]
	else
		if element:isString() then
			prototype.adapter = self.adapters.string
		elseif element:isNum() then
			prototype.adapter = self.adapters.number
		elseif element:isMap() then
			prototype.adapter = self.adapters.map
		elseif element:isList() then
			prototype.adapter = self.adapters.list
		end
	end
	return prototype
end


function EntityEditor:addUnNamedAdapterContainer(adapter, container, parentContainer, prototype)
	local outercontainer = guiManager:createWindow("DefaultWindow")
	
	local deleteButton = nil
	local deleteButtonWidth = 0
	if prototype.nodelete == nil then
		deleteButton = self:createDeleteButton("list")
		deleteButton:setProperty("UnifiedPosition", "{{0,0},{0,2}}")
		deleteButton:setProperty("Tooltip", "Delete list item");
		deleteButtonWidth = 16
		
		function removeAdapter(args)
			adapter:remove()
			outercontainer:setAlpha(0.2)
		end
		deleteButton:subscribeEvent("Clicked", removeAdapter)
	end
		
	local width = container:getWidth()
	--increase with delete button width
	width = width + CEGUI.UDim(0, deleteButtonWidth)
	outercontainer:setWidth(width)
	
	outercontainer:setHeight(container:getHeight())
	
	container:setXPosition(CEGUI.UDim(0, deleteButtonWidth))
	
	--make sure that the outer container has the same height as the inner container (so that when we add new child adapters it's updated)
	function syncWindowHeights(args)
		outercontainer:setHeight(container:getHeight())
	end
	local SizedConnection = container:subscribeEvent("Sized", syncWindowHeights)
	
	if deleteButton then
		outercontainer:addChildWindow(deleteButton)
	end
	outercontainer:addChildWindow(container)

	parentContainer:addChildWindow(outercontainer)
	return outercontainer
end

function EntityEditor:addNamedAdapterContainer(attributeName, adapter, container, parentContainer, prototype)
	local textWidth = 75
	local outercontainer = guiManager:createWindow("DefaultWindow")
	--outercontainer:setRiseOnClickEnabled(false)
	local label = guiManager:createWindow("EmberLook/StaticText")
	

	
	label:setText(attributeName)
	label:setWidth(CEGUI.UDim(0, textWidth))
	label:setProperty("FrameEnabled", "false");
 	label:setProperty("BackgroundEnabled", "false");
	label:setProperty("VertFormatting", "TopAligned");
	label:setProperty("Tooltip", attributeName);
	
	local width = container:getWidth()
	width = width + CEGUI.UDim(0, textWidth)
	outercontainer:setWidth(width)
	container:setXPosition(CEGUI.UDim(0, textWidth))
	container:setProperty("Tooltip", attributeName);
	
	outercontainer:setHeight(container:getHeight())
	
	--make sure that the outer container has the same height as the inner container (so that when we add new child adapters it's updated)
	function syncWindowHeights(args)
		outercontainer:setHeight(container:getHeight())
	end
	local SizedConnection = container:subscribeEvent("Sized", syncWindowHeights)
	
	if prototype.nodelete == nil then
	
		local deleteButton = self:createDeleteButton(attributeName)
		deleteButton:setProperty("UnifiedPosition", "{{1,-16},{0,2}}")
		deleteButton:setProperty("Tooltip", "Delete '" .. attributeName .. "'");
	
	-- 	function showDeleteButton(args)
	-- 		console:pushMessage("wee")
	-- 		deleteButton:setVisible(true)
	-- 	end
	-- 	function hideDeleteButton(args)
	-- 		console:pushMessage("waa")
	-- 		deleteButton:setVisible(false)
	-- 	end
	-- 	outercontainer:subscribeEvent("MouseEnter", showDeleteButton)
	-- 	outercontainer:subscribeEvent("MouseLeave", hideDeleteButton)
		
		function removeAdapter(args)
			adapter:remove()
			outercontainer:setAlpha(0.2)
		end
		deleteButton:subscribeEvent("Clicked", removeAdapter)
		
		label:addChildWindow(deleteButton)
	end
	
	outercontainer:addChildWindow(label)
	outercontainer:addChildWindow(container)

	parentContainer:addChildWindow(outercontainer)
	return outercontainer
end

function EntityEditor:createDeleteButton(attributeName)
	local deleteButton = guiManager:createWindow("EmberLook/SystemButton")
	deleteButton:setProperty("NormalImage", "set:EmberLook image:CloseButtonNormal")
	deleteButton:setProperty("HoverImage", "set:EmberLook image:CloseButtonHover")
	deleteButton:setProperty("PushedImage", "set:EmberLook image:CloseButtonPushed")
	deleteButton:setProperty("UnifiedSize", "{{0,16},{0,16}}")
	deleteButton:setAlpha(0.5)
	return deleteButton
end

function EntityEditor:fillNewElementCombobox(combobox, elementName, outerElement)

	combobox:resetList()
	local newAdapters = {}

	local possibleProto = self.prototypes[elementName]
	if possibleProto then
		if possibleProto.adapter then
			local itemIndex = table.maxn(newAdapters) + 1
			
			local item = Ember.OgreView.Gui.ColouredListItem:new(possibleProto.adapter.name, itemIndex)
			table.insert(newAdapters, possibleProto.adapter)
			combobox:addItem(item)
		end
	else
		--Use the default adapters
	
		for index,value in pairs(self.defaultPrototypes) do
			local itemIndex = table.maxn(newAdapters) + 1
			console:pushMessage(itemIndex)
			local item = Ember.OgreView.Gui.ColouredListItem:new(value.adapter.name, itemIndex)
			table.insert(newAdapters, value.adapter)
			combobox:addItem(item)
		end
	end
	
	--check that our previous selection is still available
	local selectedItem = combobox:findItemWithText(combobox:getText(), nil)
	if selectedItem == nil then
		if combobox:getItemCount() == 1 then
			combobox:getListboxItemFromIndex(0):setSelected(true)
			combobox:setText(combobox:getSelectedItem():getText())
		else
			combobox:clearAllSelections()
			combobox:setText("")
		end
	else
		selectedItem:setSelected(true)
		combobox:setText(combobox:getSelectedItem():getText())
	end
	
	combobox:setHeight(CEGUI.UDim(0, 100))
	combobox:setProperty("ReadOnly", "true")
-- 	--combobox:getDropList():setProperty("ClippedByParent", "false")
	return newAdapters
end



function EntityEditor:Submit_Clicked(args)
	self.instance.helper:submitChanges()
	--we want to update the next time a change comes from the server
	self.listenForChanges = true
	--self:editEntity(self.instance.entity)
	return true
end

function EntityEditor:DeleteButton_Clicked(args)
	if self.instance then
		local entity = self.instance.entity
		if entity then
		-- 	self:clearEditing()
			emberServices:getServerService():deleteEntity(entity)
		end
	end
	return true
end

function EntityEditor:RefreshAtlas_Clicked(args)
	if self.instance then
		local entity = self.instance.entity
		if entity then
			local ss = std.stringstream:new_local()
			local ss_log = std.stringstream:new_local()
			entity:dumpAttributes(ss, ss_log.__std__ostream__)
			self.widget:getWindow("AtlasTextbox"):setText(ss:str())
		end
	end
	return true
	
end

function EntityEditor:handleKnowledgeSelected(modelItem)
	if modelItem.predicate == "location" then
		_, _, x, y, z = string.find(modelItem.knowledge, "{%d*,%(([%d%-]*),([%d%-]*),([%d%-]*)%)}")
	
		if (x and y and z) then	
			local point = Ember.OgreView.Gui.EntityEditor:createPoint(tonumber(x), tonumber(y), tonumber(z))
			self.instance.helper:addMarker(point)
		else
			self.instance.helper:removeMarker()
		end
	else
		self.instance.helper:removeMarker()
	end
end

function EntityEditor:entitySayKnowledge(root)
	local rootObject = root:get()
	
	if not rootObject:hasAttr("say") then
		return
	end
	
	--message now contains what our target entity said
	local message = rootObject:getAttr("say"):asString()
	
	local modelItem = {} 
	_, _, modelItem.predicate, modelItem.subject, modelItem.knowledge = string.find(message, "The (%a*) of (%a*) is (.*)")
	if modelItem.predicate then

		local item = CEGUI.toItemEntry(windowManager:createWindow("EmberLook/ListboxItem"))
		item:setText(escapeForCEGUI(modelItem.predicate .. " : " .. modelItem.subject .. " : ".. modelItem.knowledge))
		item:subscribeEvent("SelectionChanged", function(args)
			if item:isSelected() then
				local predicate = self.widget:getWindow("NewKnowledgePredicate")
				local subject = self.widget:getWindow("NewKnowledgeSubject")
				local knowledge = self.widget:getWindow("NewKnowledgeKnowledge")
				
				predicate:setText(modelItem.predicate)
				subject:setText(modelItem.subject)
				knowledge:setText(modelItem.knowledge)
				
				self:handleKnowledgeSelected(modelItem)
			end
			
			return true
		end
		)
		
		item:setID(#self.instance.knowledge.model)
		table.insert(self.instance.knowledge.model, modelItem)
		self.knowledgelistbox:addItem(item)
	end

end

function EntityEditor:knowledgeRefresh()
	self.knowledgelistbox:resetList()
	if self.instance then
		self.instance.knowledge.model = {}
		local entity = self.instance.entity
		if entity then
			if self.instance.entitySayKnowledgeConnector then
				self.instance.entitySayKnowledgeConnector:disconnect()
			end
			self.instance.entitySayKnowledgeConnector = createConnector(entity.Say):connect(self.entitySayKnowledge, self)
			emberServices:getServerService():sayTo("list me all knowledge", entity)

			--Remove listener after five seconds			
			self.instance.entitySayKnowledgeConnectorTimer = Eris.Timeout:new_local(5000)
			self.instance.entitySayKnowledgeConnectorTimerConn = createConnector(self.instance.entitySayKnowledgeConnectorTimer.Expired):connect(function()
				self.instance.entitySayKnowledgeConnector:disconnect()
			end)
		end
	end
end

function EntityEditor:RefreshKnowledge_Clicked(args)
	self:knowledgeRefresh()
	return true
end

function EntityEditor:NewKnowledge_Clicked(args)
	local predicate = self.widget:getWindow("NewKnowledgePredicate")
	local subject = self.widget:getWindow("NewKnowledgeSubject")
	local knowledge = self.widget:getWindow("NewKnowledgeKnowledge")
	self.instance.helper:addKnowledge(predicate:getText(), subject:getText(), knowledge:getText())
	self:knowledgeRefresh()
	return true
end

function EntityEditor:entitySayGoals(root)
	local rootObject = root:get()
	
	if not rootObject:hasAttr("say") then
		return
	end
	
	--message now contains what our target entity said
	local message = rootObject:getAttr("say"):asString()
	
	local modelItem = {}
	
	_, _, modelItem.verb, modelItem.goal = string.find(message, "The goal of (%b()) is (.*)")
	if modelItem.verb then
		local item = CEGUI.toItemEntry(windowManager:createWindow("EmberLook/ListboxItem"))
		item:setText(escapeForCEGUI(modelItem.verb .. " : " .. modelItem.goal))
		self.goallistbox:addItem(item)

		item:subscribeEvent("SelectionChanged", function(args)
			if item:isSelected() then
				local goalVerb = self.widget:getWindow("NewGoalVerb")
				local goalDef = self.widget:getWindow("NewGoalDefinition")
				
				local _, _, singleVerb = string.find(modelItem.verb, "'(%a*)'.*")
				if singleVerb then
					goalVerb:setText(singleVerb)
				else
					goalVerb:setText(modelItem.verb)
				end
				
				goalDef:setText(modelItem.goal)
			end
			
			return true
		end
		)

	end

end

function EntityEditor:goalsRefresh()
	self.goallistbox:resetList()
	if self.instance then
		local entity = self.instance.entity
		if entity then
			if self.instance.entitySayGoalsConnector then
				self.instance.entitySayGoalsConnector:disconnect()
			end
			self.instance.entitySayGoalsConnector = createConnector(entity.Say):connect(self.entitySayGoals, self)
			emberServices:getServerService():sayTo("list me goal", entity)

			--Remove listener after five seconds			
			self.instance.entitySayGoalsConnectorTimer = Eris.Timeout:new_local(5000)
			self.instance.entitySayGoalsConnectorTimerConn = createConnector(self.instance.entitySayGoalsConnectorTimer.Expired):connect(function()
				self.instance.entitySayGoalsConnector:disconnect()
			end)
		end
	end
end

function EntityEditor:RefreshGoals_Clicked(args)
	self:goalsRefresh()
	return true
end

function EntityEditor:NewGoal_Clicked(args)
	local goalVerb = self.widget:getWindow("NewGoalVerb")
	local goalDef = self.widget:getWindow("NewGoalDefinition")
	self.instance.helper:addGoal(goalVerb:getText(), goalDef:getText())
	return true
end

function EntityEditor:ExportButton_Clicked(args)
	emberOgre:getWorld():getEntityFactory():dumpAttributesOfEntity(self.instance.entity:getId())
	return true
end

function EntityEditor:RefreshButton_Clicked(args)
	if self.instance.entity then
		self:editEntity(self.instance.entity)
	end
	return true
end

function EntityEditor:ShowOgreBbox_CheckStateChanged(args)
	if self.instance.entity then
		self.instance.entity:setVisualize("OgreBBox", self.modelTab.showOgreBbox:isSelected())
	end
	return true
end

function EntityEditor:ShowErisBbox_CheckStateChanged(args)
	if self.instance.entity then
		if self.modelTab.showErisBbox:isSelected() then
			self.world:getAuthoringManager():displaySimpleEntityVisualization(self.instance.entity)
		else
			self.world:getAuthoringManager():hideSimpleEntityVisualization(self.instance.entity)
		end
	end
	return true
end

function EntityEditor:ChildList_MouseDoubleClick(args)
	local entityId = self.childlistbox:getFirstSelectedItem():getID()
	editEntity(entityId)
	return true
end



function EntityEditor:handleAction(action, entity) 

	if action == "edit" then
		self:editEntity(entity)
	end
end

function EntityEditor:refreshChildren(entity)
	self.childListholder:resetList()
	local numContained = entity:numContained()
	if numContained ~= 0 then
		for i = 0, numContained - 1 do
			local childEntity = entity:getContained(i)
			local label = childEntity:getName()
			
			local item = Ember.OgreView.Gui.ColouredListItem:new(label, childEntity:getId(), childEntity)
			self.childListholder:addItem(item)
		end
	end
end

function EntityEditor:refreshModelInfo(entity)
	local showOgreBbox = entity:getVisualize("OgreBBox")
	self.modelTab.showOgreBbox:setSelected(showOgreBbox)
	local showErisBbox = self.world:getAuthoringManager():hasSimpleEntityVisualization(self.instance.entity)
	self.modelTab.showErisBbox:setSelected(showErisBbox)
end

function EntityEditor:Entity_Changed(attributes)
	--only update if we're actively listening (for example right after an update)
	if self.listenForChanges then
		self.listenForChanges = false
		self:editEntity(self.instance.entity)
	end
end

--we need to clean up when the entity is deleted, so we don't cause segfaults when trying to access a null ref
function EntityEditor:Entity_BeingDeleted()
	self:clearEditing()
end


function EntityEditor:buildWidget()

	self.factory = Ember.OgreView.Gui.Adapters.Atlas.AdapterFactory:new("EntityEditor")
	
	self.widget = guiManager:createWidget()
	self.widget:loadMainSheet("EntityEditor.layout", "EntityEditor/")
	
	self.attributesContainer = self.widget:getWindow("AttributesContainer")
	self.infoWindow = self.widget:getWindow("EntityInfo")
	
	self.childlistbox = CEGUI.toListbox(self.widget:getWindow("ChildList"))
	--EntityBrowser.childlistbox:subscribeEvent("ItemSelectionChanged", "EntityBrowser.EntityList_SelectionChanged")
	
	self.childlistFilter = CEGUI.toEditbox(self.widget:getWindow("FilterChildren"))
	self.childListholder = Ember.OgreView.Gui.ListHolder:new(self.childlistbox, self.childlistFilter)
	
	self.goallistbox = CEGUI.toItemListbox(self.widget:getWindow("GoalList"))

	self.knowledgelistbox = CEGUI.toItemListbox(self.widget:getWindow("KnowledgeList"))
		
--[[	self.modelTab.stackableWindow = self.widget:getWindow("ModelPanelStackable")
	self.modelTab.stackableContainer = Ember.OgreView.Gui.StackableContainer:new_local(self.modelTab.stackableWindow)
	self.modelTab.stackableContainer:setInnerContainerWindow(self.modelTab.stackableWindow)]]
	self.modelTab.showOgreBbox = CEGUI.toCheckbox(self.widget:getWindow("ShowOgreBbox"))
	self.modelTab.showErisBbox = CEGUI.toCheckbox(self.widget:getWindow("ShowErisBbox"))
	self.modelTab.modelInfo = self.widget:getWindow("ModelInfo")
	
	
	connect(self.connectors, guiManager.EventEntityAction, self.handleAction, self)
	
	
	self.widget:getWindow("ChildList"):subscribeEvent("DoubleClick", self.ChildList_MouseDoubleClick, self)
	self.widget:getWindow("ShowOgreBbox"):subscribeEvent("CheckStateChanged", self.ShowOgreBbox_CheckStateChanged, self)
	self.widget:getWindow("ShowErisBbox"):subscribeEvent("CheckStateChanged", self.ShowErisBbox_CheckStateChanged, self)
	self.widget:getWindow("RefreshAtlas"):subscribeEvent("Clicked", self.RefreshAtlas_Clicked, self)
	self.widget:getWindow("RefreshKnowledge"):subscribeEvent("Clicked", self.RefreshKnowledge_Clicked, self)
	self.widget:getWindow("NewKnowledgeAdd"):subscribeEvent("Clicked", self.NewKnowledge_Clicked, self)
	self.widget:getWindow("RefreshGoals"):subscribeEvent("Clicked", self.RefreshGoals_Clicked, self)
	self.widget:getWindow("NewGoalAdd"):subscribeEvent("Clicked", self.NewGoal_Clicked, self)
	self.widget:getWindow("Submit"):subscribeEvent("Clicked", self.Submit_Clicked, self)
	self.widget:getWindow("DeleteButton"):subscribeEvent("Clicked", self.DeleteButton_Clicked, self)
	self.widget:getWindow("ExportButton"):subscribeEvent("Clicked", self.ExportButton_Clicked, self)
	self.widget:getWindow("RefreshButton"):subscribeEvent("Clicked", self.RefreshButton_Clicked, self)
	
	--self.attributeStackableContainer = Ember.OgreView.Gui.StackableContainer:new_local(self.attributesContainer)
	self.widget:registerConsoleVisibilityToggleCommand("entityEditor")
	self.widget:enableCloseButton()
	self.widget:hide()

end

function EntityEditor:shutdown()
	disconnectAll(self.connectors)
	deleteSafe(self.factory)
	deleteSafe(self.childListholder)
	guiManager:destroyWidget(self.widget)
end


EntityEditor.createdWorldConnector = createConnector(emberOgre.EventWorldCreated):connect(function(world)
		entityEditor = {connectors={},
			instance = {
				stackableContainers = {},
				entity = nil,
				rootMapAdapter = nil,
				helper = nil,
				newElements = {},
				deleteListener = nil,
				model = {}
			},
			factory = nil,
			attributesContainer = nil,
			modelTab = {},
			world = world
		}
		setmetatable(entityEditor, {__index = EntityEditor})
		
		entityEditor:buildWidget()
		connect(entityEditor.connectors, emberOgre.EventWorldDestroyed, function()
				entityEditor:shutdown()
				entityEditor = nil
			end
		)
	end
)

