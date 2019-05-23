/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"



namespace ve {

	/**
	* \brief Initialize the subrenderer
	*/
	void VESubrenderFW::initSubrenderer() {
	};


	/**
	* \brief If the window size changes then some resources have to be recreated to fit the new size.
	*/
	void VESubrenderFW::recreateResources() {
		closeSubrenderer();
		initSubrenderer();

		uint32_t size = (uint32_t)m_descriptorSetsResources.size();
		if (size > 0) {
			m_descriptorSetsResources.clear();
			vh::vhRenderCreateDescriptorSets(getRendererForwardPointer()->getDevice(),
				size, m_descriptorSetLayoutResources,
				getRendererForwardPointer()->getDescriptorPool(),
				m_descriptorSetsResources);

			for (uint32_t i = 0; i < size; i++) {
				vh::vhRenderUpdateDescriptorSetMaps(getRendererPointer()->getDevice(),
					m_descriptorSetsResources[i],
					0, i*m_resourceArrayLength, m_resourceArrayLength, m_maps);
			}
		}
	}

	/**
	* \brief Close down the subrenderer and destroy all local resources.
	*/
	void VESubrenderFW::closeSubrenderer() {

		for (auto pipeline : m_pipelines) {
			vkDestroyPipeline(getRendererPointer()->getDevice(), pipeline, nullptr);
		}

		if (m_pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);

		if (m_descriptorSetLayoutResources != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayoutResources, nullptr);

	}


	/**
	* \brief Bind the subrenderer's pipeline to a commandbuffer
	*
	* \param[in] commandBuffer The command buffer to bind the pipeline to
	*
	*/
	void VESubrenderFW::bindPipeline(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);	//bind the PSO
	}


	/**
	* \brief Bind per frame descriptor sets to the pipeline layout
	*
	* \param[in] commandBuffer The command buffer that is used for recording commands
	* \param[in] imageIndex The index of the swapchain image that is currently used
	* \param[in] pCamera Pointer to the current light camera
	* \param[in] pLight Pointer to the currently used light
	* \param[in] descriptorSetsShadow Shadow maps that are used for creating shadow
	*
	*/
	void VESubrenderFW::bindDescriptorSetsPerFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex,
		VECamera *pCamera, VELight *pLight,
		std::vector<VkDescriptorSet> descriptorSetsShadow) {

		//set 0...cam UBO
		//set 1...light resources
		//set 2...shadow maps
		//set 3...per object UBO
		//set 4...additional per object resources

		std::vector<VkDescriptorSet> set = {
			pCamera->m_memoryHandle.pMemBlock->descriptorSets[imageIndex],
			pLight->m_memoryHandle.pMemBlock->descriptorSets[imageIndex]
		};

		uint32_t offsets[2] = { pCamera->m_memoryHandle.entryIndex * sizeof(VECamera::veUBOPerCamera_t),
			pLight->m_memoryHandle.entryIndex * sizeof(VELight::veUBOPerLight_t) };

		if (descriptorSetsShadow.size()>0) {
			set.push_back(descriptorSetsShadow[imageIndex]);
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
			0, (uint32_t)set.size(), set.data(), 2, offsets);

	}


	/**
	*
	* \brief Bind default descriptor sets
	*
	* The function binds the default descriptor sets. Can be overloaded.
	*
	* \param[in] commandBuffer The command buffer to record into all draw calls
	* \param[in] imageIndex Index of the current swap chain image
	* \param[in] entity Pointer to the entity to draw
	*
	*/
	void VESubrenderFW::bindDescriptorSetsPerEntity(VkCommandBuffer commandBuffer, uint32_t imageIndex, VEEntity *entity) {

		//set 0...cam UBO
		//set 1...light resources
		//set 2...shadow maps
		//set 3...per object UBO
		//set 4...additional per object resources

		std::vector<VkDescriptorSet> sets = { entity->m_memoryHandle.pMemBlock->descriptorSets[imageIndex] };
		if (m_descriptorSetsResources.size() > 0 && entity->getResourceIdx() % m_resourceArrayLength == 0) {
			sets.push_back(m_descriptorSetsResources[entity->getResourceIdx() / m_resourceArrayLength]);
		}

		uint32_t offset = entity->m_memoryHandle.entryIndex * sizeof(VEEntity::veUBOPerObject_t);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
			3, (uint32_t)sets.size(), sets.data(), 1, &offset);

	}


	/**
	* \brief Draw all associated entities.
	*
	* The subrenderer maintains a list of all associated entities. In this function it goes through all of them
	* and draws them. A vector is used in order to be able to parallelize this in case thousands or objects are in the list
	*
	* \param[in] commandBuffer The command buffer to record into all draw calls
	* \param[in] imageIndex Index of the current swap chain image
	* \param[in] numPass The number of the light that has been rendered
	* \param[in] pCamera Pointer to the current light camera
	* \param[in] pLight Pointer to the current light
	* \param[in] descriptorSetsShadow The shadow maps to be used.
	*
	*/
	void VESubrenderFW::draw(VkCommandBuffer commandBuffer, uint32_t imageIndex,
		uint32_t numPass,
		VECamera *pCamera, VELight *pLight,
		std::vector<VkDescriptorSet> descriptorSetsShadow) {

		if (m_entities.size() == 0) return;

		if (numPass > 0 && getClass() != VE_SUBRENDERER_CLASS_OBJECT) return;

		bindPipeline(commandBuffer);

		setDynamicPipelineState(commandBuffer, numPass);

		bindDescriptorSetsPerFrame(commandBuffer, imageIndex, pCamera, pLight, descriptorSetsShadow);

		//go through all entities and draw them
		for (auto pEntity : m_entities) {
			if (pEntity->m_drawEntity) {
				bindDescriptorSetsPerEntity(commandBuffer, imageIndex, pEntity);	//bind the entity's descriptor sets
				drawEntity(commandBuffer, imageIndex, pEntity);
			}
		}
	}

	/**
	*
	* \brief Draw one entity
	*
	* The function binds the vertex buffer, index buffer, and descriptor set of the entity, then commits a draw call
	*
	* \param[in] commandBuffer The command buffer to record into all draw calls
	* \param[in] imageIndex Index of the current swap chain image
	* \param[in] entity Pointer to the entity to draw
	*
	*/
	void VESubrenderFW::drawEntity(VkCommandBuffer commandBuffer, uint32_t imageIndex, VEEntity *entity) {

		VkBuffer vertexBuffers[] = { entity->m_pMesh->m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);	//bind vertex buffer

		vkCmdBindIndexBuffer(commandBuffer, entity->m_pMesh->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32); //bind index buffer

		vkCmdDrawIndexed(commandBuffer, entity->m_pMesh->m_indexCount, 1, 0, 0, 0); //record the draw call
	}



	/**
	*
	* \brief Add some maps of an entity to the map list of this subrenderer
	*
	* \param[in] pEntity Pointer to the entity
	* \param[in] newMaps List of maps to be added to this subrenderer
	*
	*/
	void VESubrenderFW::addMaps(VEEntity *pEntity, std::vector<VkDescriptorImageInfo> &newMaps) {
		pEntity->setResourceIdx((uint32_t)m_entities.size());

		uint32_t offset = 0;
		if (pEntity->getResourceIdx() % m_resourceArrayLength == 0) {
			vh::vhRenderCreateDescriptorSets(getRendererForwardPointer()->getDevice(),
				1, m_descriptorSetLayoutResources,
				getRendererForwardPointer()->getDescriptorPool(),
				m_descriptorSetsResources);

			offset = (uint32_t)m_maps[0].size();

			for (uint32_t i = 0; i < m_maps.size(); i++) {
				m_maps[i].resize(offset + m_resourceArrayLength);
				for (uint32_t j = offset; j < offset + m_resourceArrayLength; j++) m_maps[i][j] = newMaps[i];
			}
		}
		else {
			offset = (uint32_t)m_maps[0].size() / m_resourceArrayLength - 1;
			offset *= m_resourceArrayLength;
			for (uint32_t i = 0; i < m_maps.size(); i++) {
				m_maps[i][m_entities.size()] = newMaps[i];
			}
		}

		vh::vhRenderUpdateDescriptorSetMaps(getRendererPointer()->getDevice(),
			m_descriptorSetsResources[m_descriptorSetsResources.size() - 1],
			0, offset, m_resourceArrayLength, m_maps);

	}


	/**
	*
	* \brief Removes an entity from this subrenderer - does NOT delete it
	*
	* Since we use indices and each entity knows its onw index, this is an O(1) operation.
	*
	* \param[in] pEntity Pointer to the entity to be removed
	*
	*/
	void VESubrenderFW::removeEntity(VEEntity *pEntity) {

		uint32_t size = (uint32_t)m_entities.size();
		if (size == 0) return;

		for (uint32_t i = 0; i < size; i++) {
			if (m_entities[i] == pEntity) {
				m_entities[i] = m_entities[size - 1];			//replace with former last entity (could be identical)
				m_entities[i]->setResourceIdx(i);				//new resource index
				for (uint32_t j = 0; j < m_maps.size(); j++) {	//move also the map entries
					m_maps[j][i] = m_maps[j][size - 1];
				}
				uint32_t offset = (uint32_t)(i / m_resourceArrayLength) - 1;
				offset *= m_resourceArrayLength;
				vh::vhRenderUpdateDescriptorSetMaps(getRendererPointer()->getDevice(),
					m_descriptorSetsResources[(uint32_t)(i / m_resourceArrayLength)],
					0, offset, m_resourceArrayLength, m_maps);

				m_entities.pop_back();				//remove the last
				if (m_entities.size() % m_resourceArrayLength == 0) {	//shrunk?
					for (uint32_t j = 0; j < m_maps.size(); j++) {
						m_maps[j].resize(m_entities.size());			//remove map entries
					}
					m_descriptorSetsResources.pop_back();				//remove descriptor set
				}
			}
		}
	}
}
