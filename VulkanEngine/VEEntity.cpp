/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"



namespace ve {

	VkDescriptorSetLayout VESceneObject::m_descriptorSetLayoutPerObject = VK_NULL_HANDLE;	///<Descriptor set layout per object

	//---------------------------------------------------------------------
	//Scene node


	/**
	* \brief The constructor ot the VESceneNode class
	*
	* \param[in] name The name of this node.
	* \param[in] transf Position and orientation.
	* \param[in] parent A parent.
	*
	*/
	VESceneNode::VESceneNode(std::string name, glm::mat4 transf, VESceneNode *parent) : VENamedClass(name) {
		m_parent = parent;
		if (parent != nullptr) {
			parent->addChild(this);
		}
		setTransform(transf);			//sets this MO also onto the dirty list to be updated
	}

	VESceneNode::~VESceneNode() {
	}


	/**
	* \returns the entity's local to parent transform.
	*/
	glm::mat4 VESceneNode::getTransform() {
		return m_transform;
	}

	/**
	* \brief Sets the entity's local to parent transform.
	*/
	void VESceneNode::setTransform(glm::mat4 trans) {
		m_transform = trans;
		update();
	}

	/**
	* \brief Sets the entity's position.
	*/
	void VESceneNode::setPosition(glm::vec3 pos) {
		m_transform[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
		update();
	};

	/**
	* \returns the entity's position.
	*/
	glm::vec3 VESceneNode::getPosition() {
		return glm::vec3(m_transform[3].x, m_transform[3].y, m_transform[3].z);
	};

	/**
	* \returns the entity's local x-axis in parent space
	*/
	glm::vec3 VESceneNode::getXAxis() {
		glm::vec4 x = m_transform[0];
		return glm::vec3(x.x, x.y, x.z);
	}

	/**
	* \returns the entity's local y-axis in parent space
	*/
	glm::vec3 VESceneNode::getYAxis() {
		glm::vec4 y = m_transform[1];
		return glm::vec3(y.x, y.y, y.z);
	}

	/**
	* \returns the entity's local z-axis in parent space
	*/
	glm::vec3 VESceneNode::getZAxis() {
		glm::vec4 z = m_transform[2];
		return glm::vec3(z.x, z.y, z.z);
	}

	/**
	*
	* \brief Multiplies the entity's transform with another 4x4 transform.
	*
	* The transform can be a translation, sclaing, rotation etc.
	*
	* \param[in] trans The 4x4 transform that is multiplied from the left onto the entity's old transform.
	*
	*/
	void VESceneNode::multiplyTransform(glm::mat4 trans) {
		setTransform(trans*m_transform);
	};

	/**
	*
	* \brief An entity's world matrix is the local to parent transform multiplied by the parent's world matrix.
	*
	* \returns the entity's world (aka model) matrix.
	*
	*/
	glm::mat4 VESceneNode::getWorldTransform() {
		if (m_parent != nullptr) return m_parent->getWorldTransform() * m_transform;
		return m_transform;
	};


	/**
	*
	* \brief lookAt function
	*
	* \param[in] eye New position of the entity
	* \param[in] point Entity looks at this point (= new local z axis)
	* \param[in] up Up vector pointing up
	*
	*/
	void VESceneNode::lookAt(glm::vec3 eye, glm::vec3 point, glm::vec3 up) {
		m_transform[3] = glm::vec4(eye.x, eye.y, eye.z, 1.0f);
		glm::vec3 z = glm::normalize(point - eye);
		up = glm::normalize(up);
		float corr = glm::dot(z, up);	//if z, up are lined up (corr=1 or corr=-1), decorrelate them
		if (1.0f - fabs(corr) < 0.00001f) {
			float sc = z.x + z.y + z.z;
			up = glm::normalize(glm::vec3(sc, sc, sc));
		}

		m_transform[2] = glm::vec4(z.x, z.y, z.z, 0.0f);
		glm::vec3 x = glm::normalize(glm::cross(up, z));
		m_transform[0] = glm::vec4(x.x, x.y, x.z, 0.0f);
		glm::vec3 y = glm::normalize(glm::cross(z, x));
		m_transform[1] = glm::vec4(y.x, y.y, y.z, 0.0f);

	}


	/**
	*
	* \brief Adds a child object to the list of children.
	*
	* \param[in] pObject Pointer to the new child.
	*
	*/
	void VESceneNode::addChild(VESceneNode * pObject) {
		m_children.push_back(pObject);
	}

	/**
	*
	* \brief remove a child from the children list - child is NOT destroyed
	*
	* \param[in] pEntity Pointer to the child to be removed.
	*
	*/
	void VESceneNode::removeChild(VESceneNode *pEntity) {
		for (uint32_t i = 0; i < m_children.size(); i++) {
			if (pEntity == m_children[i]) {
				VESceneNode *last = m_children[m_children.size() - 1];
				m_children[i] = last;
				m_children.pop_back();		//child is not detroyed
				return;
			}
		}
	}

	/**
	*
	* \brief Update the entity's UBO buffer with the current world matrix
	*
	* If there is a parent, get the parent's world matrix. If not, set the parent matrix to identity.
	* Then call update(parent) to do the job.
	*
	*/
	void VESceneNode::update() {
		glm::mat4 parentWorldMatrix = glm::mat4(1.0);
		if (m_parent != nullptr) {
			parentWorldMatrix = m_parent->getWorldTransform();
		}
		update(parentWorldMatrix );
	}

	/**
	*
	* \brief Update the entity's UBO buffer with the current world matrix
	*
	* Calculate the new world matrix (and inv transpose matix to transform normal vectors).
	* Then copy the struct content into the UBO.
	*
	* \param[in] parentWorldMatrix The parent's world matrix or an identity matrix.
	*
	*/
	void VESceneNode::update(glm::mat4 parentWorldMatrix ) {
		glm::mat4 worldMatrix = parentWorldMatrix * getTransform();		//get world matrix
		updateUBO( worldMatrix );										//call derived class for specific data like object color
		updateChildren( worldMatrix );									//update all children
	}


	/**
	* \brief Update the UBOs of all children of this entity
	*/
	void VESceneNode::updateChildren(glm::mat4 worldMatrix ) {
		for (auto pObject : m_children) {
			pObject->update(worldMatrix);	//update the children by giving them the current worldMatrix
		}
	}

	/**
	* \brief Get a default bounding sphere for this scene node
	* \param[out] center The sphere center is also the position of the scene node
	* \param[out] radius The default radius of the sphere
	*/
	void VESceneNode::getBoundingSphere(glm::vec3 *center, float *radius) {
		*center = getPosition();
		*radius = 1.0f;
	}

	/**
	*
	* \brief Get an OBB that exactly holds the given points.
	*
	* The OBB is oriented along the local axes of the entity.
	*
	* \param[in] points The points that should be engulfed by the OBB, in world space
	* \param[in] t1 Used for interpolating between frustum edge points
	* \param[in] t2 Used for interpolating between frustum edge points
	* \param[out] center Center of the OBB
	* \param[out] width Width of the OBB
	* \param[out] height Height of the OBB
	* \param[out] depth Depth of the OBB
	*
	*/

	void VESceneNode::getOBB(std::vector<glm::vec4> &points, float t1, float t2,
		glm::vec3 &center, float &width, float &height, float &depth) {

		glm::mat4 W = getWorldTransform();

		std::vector<glm::vec4> axes;		//3 local axes, into pos and minus direction
		axes.push_back(-1.0f*W[0]);
		axes.push_back(W[0]);
		axes.push_back(-1.0f*W[1]);
		axes.push_back(W[1]);
		axes.push_back(-1.0f*W[2]);
		axes.push_back(W[2]);

		std::vector<glm::vec4> box;			//maxima points into the 6 directions
		std::vector<float> maxvalues;		//max ordinates
		box.resize(6);
		maxvalues.resize(6);
		for (uint32_t i = 0; i < 6; i++) {	//fill maxima with first point
			box[i] = points[0];
			maxvalues[i] = glm::dot(axes[i], points[0]);
		}

		for (uint32_t i = 1; i < points.size(); i++) {		//go through rest of the points and 6 axis directions
			for (uint32_t j = 0; j < 6; j++) {
				float tmp = glm::dot(axes[j], points[i]);
				if (maxvalues[j] < tmp) {
					box[j] = points[i];
					maxvalues[j] = tmp;
				}
			}
		}
		width = maxvalues[1] + maxvalues[0];
		height = maxvalues[3] + maxvalues[2];
		depth = maxvalues[5] + maxvalues[4];
		glm::vec4 center4 = W * glm::vec4(-maxvalues[0] + width / 2.0f,
			-maxvalues[2] + height / 2.0f,
			-maxvalues[4] + depth / 2.0f, 0.0f);
		center = glm::vec3(center4.x, center4.y, center4.z);
	}



	//-----------------------------------------------------------------------------------------------------
	//Scene object

	VESceneObject::VESceneObject(std::string name, glm::mat4 transf, VESceneNode *parent, uint32_t sizeUBO ) : 
									VESceneNode(name, transf, parent) {

		if ( m_descriptorSetLayoutPerObject == VK_NULL_HANDLE ) {
			vh::vhRenderCreateDescriptorSetLayout(getRendererForwardPointer()->getDevice(),
				{ 1 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
				&m_descriptorSetLayoutPerObject);
		}

		if (sizeUBO > 0) {
			vh::vhBufCreateUniformBuffers(	getRendererPointer()->getVmaAllocator(),
											(uint32_t)getRendererPointer()->getSwapChainNumber(),
											sizeUBO, m_uniformBuffers, m_uniformBuffersAllocation);

			vh::vhRenderCreateDescriptorSets(getRendererForwardPointer()->getDevice(),
				(uint32_t)getRendererForwardPointer()->getSwapChainNumber(),
				m_descriptorSetLayoutPerObject,
				getRendererForwardPointer()->getDescriptorPool(),
				m_descriptorSetsUBO);

			for (uint32_t i = 0; i < m_descriptorSetsUBO.size(); i++) {
				vh::vhRenderUpdateDescriptorSet(getRendererForwardPointer()->getDevice(),
					m_descriptorSetsUBO[i],
					{ m_uniformBuffers[i] },		//UBOs
					{ sizeof(sizeUBO) },			//UBO sizes
					{ { VK_NULL_HANDLE } },			//textureImageViews
					{ { VK_NULL_HANDLE } }			//samplers
				);
			}
		}
	}


	VESceneObject::~VESceneObject() {
		for (uint32_t i = 0; i < m_uniformBuffers.size(); i++) {
			vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_uniformBuffers[i], m_uniformBuffersAllocation[i]);
		}
	}


	void VESceneObject::updateUBO(void *pUBO, uint32_t sizeUBO) {
		//uint32_t imageIndex = getRendererPointer()->getImageIndex();	//TODO: current swap chain image!!!!!!

		for (uint32_t i = 0; i < getRendererPointer()->getSwapChainNumber(); i++) {
			void* data = nullptr;
			vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uniformBuffersAllocation[i], &data);
			memcpy(data, pUBO, sizeUBO);
			vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uniformBuffersAllocation[i]);
		}
	}




	//---------------------------------------------------------------------
	//Entity


	/**
	*
	* \brief VEEntity constructor.
	*
	* Create a VEEntity from a mesh, material, using a transform and a parent.
	*
	* \param[in] name The name of the mesh.
	* \param[in] type Mesh type
	* \param[in] pMesh Pointer to the mesh.
	* \param[in] pMat Pointer to the material.
	* \param[in] transf The local to parent transform.
	* \param[in] parent Pointer to the entity's parent.
	*
	*/
	VEEntity::VEEntity(	std::string name, veEntityType type, 
						VEMesh *pMesh, VEMaterial *pMat, 
						glm::mat4 transf, VESceneNode *parent) :
							VESceneObject(name, transf, parent, (uint32_t) sizeof(veUBOPerObject_t)), m_entityType( type ) {

		setTransform(transf);

		if (pMesh != nullptr && pMat != nullptr) {
			m_pMesh = pMesh;
			m_pMaterial = pMat;
			m_drawEntity = true;
			m_castsShadow = true;
		}
	}


	/**
	*
	* \brief VEEntity destructor.
	*
	* Destroy the entity's UBOs.
	*
	*/
	VEEntity::~VEEntity() {
	}

	/**
	* \brief Sets the object parameter vector.
	*
	* This causes an update of the UBO and all children.
	*
	* \param[in] param The new parameter vector
	*/
	void VEEntity::setTexParam(glm::vec4 param) {
		m_texParam = param;
		update();
	}


	/**
	*
	* \brief Update the entity's UBO.
	*
	* \param[in] worldMatrix The new world matrix of the entity
	*
	*/
	void VEEntity::updateUBO( glm::mat4 worldMatrix) {
		veUBOPerObject_t ubo = {};

		ubo.model = worldMatrix;
		ubo.modelInvTrans = glm::transpose(glm::inverse(worldMatrix));
		ubo.texParam = m_texParam;
		if (m_pMaterial != nullptr) {
			ubo.color = m_pMaterial->color;
		};

		VESceneObject::updateUBO( (void*)&ubo, (uint32_t)sizeof(veUBOPerObject_t));
	}


	/**
	* \brief Get a bounding sphere for this entity
	*
	* Return the bounding sphere of the mesh that this entity represents (if there is one).
	* 
	* \param[out] center Pointer to the sphere center to return
	* \param[out] radius Pointer to the radius to return
	*
	*/
	void VEEntity::getBoundingSphere(glm::vec3 *center, float *radius) {
		*center = getPosition();
		*radius = 1.0f;
		if (m_pMesh != nullptr) {
			*center = m_pMesh->m_boundingSphereCenter;
			*radius = m_pMesh->m_boundingSphereRadius;
		}
	}



	//-------------------------------------------------------------------------------------------------
	//camera

	/**
	*
	* \brief VECamera constructor. Set nearPlane, farPlane to a default value.
	*
	* \param[in] name Name of the camera.
	*
	*/
	VECamera::VECamera(std::string name, glm::mat4 transf, VESceneNode *parent ) : 
							VESceneObject(name, transf, parent, (uint32_t)sizeof(veUBOPerCamera_t)) {
	};

	/**
	*
	* \brief VECamera constructor. 
	*
	* \param[in] name Name of the camera.
	* \param[in] nearPlane Distance of near plane to the camera origin
	* \param[in] farPlane Distance of far plane to the camera origin
	*
	*/
	VECamera::VECamera(	std::string name, 
						float nearPlane, float farPlane,
						float nearPlaneFraction, float farPlaneFraction,
						glm::mat4 transf, VESceneNode *parent ) :
							VESceneObject(name, transf, parent, (uint32_t)sizeof(veUBOPerCamera_t)), 
							m_nearPlane(nearPlane), m_farPlane(farPlane),
							m_nearPlaneFraction(nearPlaneFraction), m_farPlaneFraction(farPlaneFraction) {
	}


	void VECamera::updateUBO(glm::mat4 worldMatrix) {
		veUBOPerCamera_t ubo = {};

		ubo.model = worldMatrix;
		ubo.view = glm::inverse(worldMatrix);
		ubo.proj = getProjectionMatrix();
		ubo.param[0] = m_nearPlane;
		ubo.param[1] = m_farPlane;
		ubo.param[2] = m_nearPlaneFraction;		//needed only if this is a shadow cam
		ubo.param[3] = m_farPlaneFraction;		//needed only if this is a shadow cam

		VESceneObject::updateUBO((void*)&ubo, (uint32_t)sizeof(veUBOPerCamera_t));
	}



	/**
	*
	* \brief Fills a veCameraData_t structure with the data of this camera
	*
	* In this case the camera is used in the light pass to capture photos of light.
	*
	* \param[out] pCamera Pointer to a veCameraData_t struct that will be filled.
	*
	*/
	/*void VECamera::fillCameraStructure(veCameraData_t *pCamera) {
		pCamera->camModel = getWorldTransform();
		pCamera->camView = glm::inverse(pCamera->camModel);

		VkExtent2D extent = getWindowPointer()->getExtent();
		pCamera->camProj = getProjectionMatrix( (float)extent.width, (float)extent.height);
		pCamera->param[0] = m_nearPlane;
		pCamera->param[1] = m_farPlane;
	}*/


	/**
	*
	* \brief Fills a veShadowData_t structure with the data of this camera
	*
	* In this scenario the camera is used to create shadow maps, i.e., it is 
	* used in a shadow pass.
	*
	* \param[out] pShadow Pointer to a veShadowData_t struct that will be filled.
	*
	*/
	/*void VECamera::fillShadowStructure(veShadowData_t *pShadow ) {
		pShadow->shadowView = glm::inverse(getWorldTransform());
		pShadow->shadowProj = getProjectionMatrix();
	}*/


	/**
	* \brief Get a bounding sphere for this camera
	*
	* Return the bounding sphere of camera frustum.
	*
	* \param[out] center Pointer to the sphere center to return
	* \param[out] radius Pointer to the radius to return
	*
	*/
	void VECamera::getBoundingSphere(glm::vec3 *center, float *radius) {
		std::vector<glm::vec4> points;

		getFrustumPoints(points);					//get frustum points in world space

		glm::vec4 mean(0.0f, 0.0f, 0.0f, 1.0f);
		for (auto point : points) {
			mean += point;
		}
		mean /= (float)points.size();

		float maxsq = 0.0f;
		for (auto point : points) {
			float sq = glm::dot(mean - point, mean - point);
			maxsq = sq > maxsq ? sq : maxsq;
		}

		*center = glm::vec3( mean.x, mean.y, mean.z );
		*radius = sqrt(maxsq);
	}


	/**
	*
	* \brief Create an shadow camera 
	*
	* \param[in] pLight Pointer to a light that defines the light direction and type.
	* \param[in] z0 Startparameter for interpolating the frustum
	* \param[in] z1 Endparameter for interlopating the frustum
	* \returns a new VECamera shadow camera
	*
	*/
	VECamera * VECamera::createShadowCamera(VELight *pLight, float z0, float z1 ) {

		if (pLight->getLightType() == VELight::VE_LIGHT_TYPE_DIRECTIONAL)
			return createShadowCameraOrtho(pLight, z0, z1);

		return createShadowCameraProjective(pLight, z0, z1);
	}


	/**
	*
	* \brief Create an ortho shadow camera from the camera's frustum bounding sphere
	*
	* \param[in] pLight Pointer to a light that defines the light direction.
	* \param[in] z0 Startparameter for interpolating the frustum
	* \param[in] z1 Endparameter for interlopating the frustum
	* \returns a new VECameraOrtho that can be used to create shadow maps for directional light
	*
	*/
	VECamera * VECamera::createShadowCameraOrtho(VELight *pLight, float z0, float z1) {

		std::vector<glm::vec4> pointsW;
		getFrustumPoints(pointsW, z0, z1);

		glm::vec3 center;
		float width, height, depth;
		pLight->getOBB(pointsW, 0.0f, 1.0f, center, width, height, depth);
		depth *= 5.0f;	//TODO - do NOT set too high or else shadow maps wont get drawn!
		VECameraOrtho *pCamOrtho = new VECameraOrtho("Ortho", 0.1f, depth, width, height);
		glm::mat4 W = pLight->getWorldTransform();
		pCamOrtho->setTransform( pLight->getWorldTransform());
		pCamOrtho->setPosition(center - depth*0.9f * glm::vec3(W[2].x, W[2].y, W[2].z));

		return pCamOrtho;
	}

	/**
	*
	* \brief Create an ortho shadow camera from the camera's frustum bounding sphere
	*
	* \param[in] pLight Pointer to a light that defines the light direction.
	* \param[in] z0 Startparameter for interpolating the frustum
	* \param[in] z1 Endparameter for interlopating the frustum
	* \returns a new VECameraProjective that can be used to create shadow maps for spot light
	* 
	*/
	VECamera * VECamera::createShadowCameraProjective(VELight *pLight, float z0, float z1) {

		glm::vec3 center;
		float radius;
		getBoundingSphere(&center, &radius);
		float diam = 2.0f * radius;

		glm::vec3 z = normalize(getZAxis());	//direction of light

		glm::vec3 pos = pLight->getPosition();	//position of light
		float pz = glm::dot(pos, z);			//z ordinate of position along the z axis

		glm::vec3 begin = center - radius * z;	//begin of frustum bounding sphere along z-axis
		float bz = glm::dot(begin, z);			//z ordinate of begin along the z axis
		float onear = bz - pz;					//near plane distance of shadow cam
		if (onear <= 0.0) onear = 0.1f;

		glm::vec3 end = center + radius * z;	//end of frustum bounding sphere along z-axis
		float ez = glm::dot(end, z);			//z ordinate of end along the z axis
		float ofar = ez - pz;					//far plane distance of shadow cam
		if (ofar <= 0.0) ofar = 1.0f;

		float fov = 45.0f*2.0f/360.0f;			//if light position is outside the sphere
		if (pz<bz) {
			float cz = glm::dot(center, z);		//z ordinate of sphere center
			float fov = atan(radius / (cz - pz));
		}

		VECameraProjective *pCamProj = new VECameraProjective("Proj", onear, ofar, 1.0f, fov);
		pCamProj->lookAt(pos, pos + ofar*z, glm::vec3(0.0f, 1.0f, 0.0f));

		return pCamProj;
	}



	//-------------------------------------------------------------------------------------------------
	//camera projective

	/**
	*
	* \brief VECameraProjective constructor. Set nearPlane, farPlane, aspect ratio and fov to a default value.
	*
	* \param[in] name Name of the camera.
	*
	*/
	VECameraProjective::VECameraProjective(std::string name, glm::mat4 transf, VESceneNode *parent) : VECamera(name, transf, parent ) {
	};

	/**
	*
	* \brief VECameraProjective constructor.
	*
	* \param[in] name Name of the camera.
	* \param[in] nearPlane Distance of near plane to the camera origin
	* \param[in] farPlane Distance of far plane to the camera origin
	* \param[in] aspectRatio Ratio between width and height. Can change due to window size change.
	* \param[in] fov Vertical field of view angle.
	*
	*/
	VECameraProjective::VECameraProjective(	std::string name, 
											float nearPlane, float farPlane, 
											float aspectRatio, float fov,
											float nearPlaneFraction, float farPlaneFraction,
											glm::mat4 transf, VESceneNode *parent) :
												VECamera(	name, 
															nearPlane, farPlane, 
															nearPlaneFraction, farPlaneFraction,
															transf, parent ), 
												m_aspectRatio(aspectRatio), m_fov(fov)   {
	};

	/**
	* \brief Get a projection matrix for this camera.
	* \param[in] width Width of the current game window.
	* \param[in] height Height of the current game window.
	* \returns the camera projection matrix.
	*/
	glm::mat4 VECameraProjective::getProjectionMatrix(float width, float height) {
		m_aspectRatio = width / height;
		glm::mat4 pm = glm::perspectiveFov( glm::radians(m_fov), (float) width, (float)height, m_nearPlane, m_farPlane);
		pm[1][1] *= -1.0f;
		pm[2][2] *= -1.0f;		//camera looks down its positive z-axis, OpenGL function does it reverse
		pm[2][3] *= -1.0f;
		return pm;
	}

	/**
	* \brief Get a projection matrix for this camera.
	* \returns the camera projection matrix.
	*/
	glm::mat4 VECameraProjective::getProjectionMatrix() {
		return getProjectionMatrix( m_aspectRatio, 1.0f );
	}

	/**
	* \brief Get a list of 8 points making up the camera frustum
	*
	* The points are returned in world space.
	*
	* \param[in] z0 Startparameter for interpolating the frustum
	* \param[in] z1 Endparameter for interlopating the frustum
	* \param[out] points List of 8 points that make up the interpolated frustum in world space
	*
	*/
	void VECameraProjective::getFrustumPoints(std::vector<glm::vec4> &points, float z0, float z1) {
		float halfh = (float)tan( (m_fov/2.0f) * M_PI / 180.0f );
		float halfw = halfh * m_aspectRatio;

		glm::mat4 W = getWorldTransform();

		points.push_back( W*glm::vec4(-m_nearPlane * halfw, -m_nearPlane * halfh, m_nearPlane, 1.0f ) );
		points.push_back( W*glm::vec4( m_nearPlane * halfw, -m_nearPlane * halfh, m_nearPlane, 1.0f));
		points.push_back( W*glm::vec4(-m_nearPlane * halfw,  m_nearPlane * halfh, m_nearPlane, 1.0f));
		points.push_back( W*glm::vec4( m_nearPlane * halfw,  m_nearPlane * halfh, m_nearPlane, 1.0f));

		points.push_back( W*glm::vec4(-m_farPlane * halfw, -m_farPlane * halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4( m_farPlane * halfw, -m_farPlane * halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4(-m_farPlane * halfw,  m_farPlane * halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4( m_farPlane * halfw,  m_farPlane * halfh, m_farPlane, 1.0f));

		for (uint32_t i = 0; i < 4; i++) {						//interpolate with z0, z1 in [0,1]
			glm::vec4 diff = points[i + 4] - points[i + 0];
			points[i + 4] = points[i + 0] + z1*diff;
			points[i + 0] = points[i + 0] + z0*diff;
		}
	}


	//-------------------------------------------------------------------------------------------------
	//camera ortho

	/**
	*
	* \brief VECameraOrtho constructor. Set nearPlane, farPlane, aspect ratio and fov to a default value.
	*
	* \param[in] name Name of the camera.
	*
	*/
	VECameraOrtho::VECameraOrtho(std::string name, glm::mat4 transf, VESceneNode *parent) : 
						VECamera(name, transf, parent ) {
	};

	/**
	*
	* \brief VECameraProjective constructor.
	*
	* \param[in] name Name of the camera.
	* \param[in] nearPlane Distance of near plane to the camera origin
	* \param[in] farPlane Distance of far plane to the camera origin
	* \param[in] width Width of the frustum
	* \param[in] height Height of the frustum
	*
	*/
	VECameraOrtho::VECameraOrtho(	std::string name, 
									float nearPlane, float farPlane, 
									float width, float height,
									float nearPlaneFraction, float farPlaneFraction,
									glm::mat4 transf, VESceneNode *parent) :
										VECamera(	name, 
													nearPlane, farPlane, 
													nearPlaneFraction, farPlaneFraction,
													transf, parent), 
										m_width(width), m_height(height) {
	};


	/**
	* \brief Get a projection matrix for this camera.
	* \param[in] width Width of the current game window.
	* \param[in] height Height of the current game window.
	* \returns the camera projection matrix.
	*/
	glm::mat4 VECameraOrtho::getProjectionMatrix(float width, float height) {
		glm::mat4 pm = glm::ortho(-width * m_width / 2.0f, width * m_width / 2.0f, -height * m_height / 2.0f, height * m_height / 2.0f, m_nearPlane, m_farPlane);
		pm[1][1] *= -1.0f;
		pm[2][2] *= -1.0;	//camera looks down its positive z-axis, OpenGL function does it reverse
		return pm;
	}

	/**
	* \brief Get a projection matrix for this camera.
	* \returns the camera projection matrix.
	*/
	glm::mat4 VECameraOrtho::getProjectionMatrix() {
		glm::mat4 pm = glm::ortho( -m_width/2.0f, m_width/2.0f, -m_height/2.0f, m_height/2.0f, m_nearPlane, m_farPlane);
		pm[2][2] *= -1;		//camera looks down its positive z-axis, OpenGL function does it reverse
		return pm;
	}


	/**
	* \brief Get a list of 8 points making up the camera frustum
	*
	* The points are returned in world space.
	*
	* \param[in] t1 Startparameter for interpolating the frustum
	* \param[in] t2 Endparameter for interlopating the frustum
	* \param[out] points List of 8 points that make up the interpolated frustum in world space
	*
	*/
	void VECameraOrtho::getFrustumPoints(std::vector<glm::vec4> &points, float t1, float t2) {
		float halfh = m_height / 2.0f;
		float halfw = m_width / 2.0f;

		glm::mat4 W = getWorldTransform();

		points.push_back( W*glm::vec4(-halfw, -halfh, m_nearPlane, 1.0f));
		points.push_back( W*glm::vec4( halfw, -halfh, m_nearPlane, 1.0f));
		points.push_back( W*glm::vec4(-halfw,  halfh, m_nearPlane, 1.0f));
		points.push_back( W*glm::vec4( halfw,  halfh, m_nearPlane, 1.0f));

		points.push_back( W*glm::vec4(-halfw, -halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4( halfw, -halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4(-halfw,  halfh, m_farPlane, 1.0f));
		points.push_back( W*glm::vec4( halfw,  halfh, m_farPlane, 1.0f));

		for (uint32_t i = 0; i < 4; i++) {						//interpolate
			glm::vec4 diff = points[i + 4] - points[i + 0];
			points[i + 4] = points[i + 0] + t2*diff;
			points[i + 0] = points[i + 0] + t1*diff;
		}
	}


	//-------------------------------------------------------------------------------------------------
	//light

	/**
	* \brief Simple VELight constructor, default is directional light
	*
	* \param[in] name Name of the camera
	*
	*/
	VELight::VELight(std::string name, glm::mat4 transf, VESceneNode *parent ) : 
						VESceneObject(name, transf, parent, (uint32_t)sizeof(veUBOPerLight_t)) {
	};


	/**
	*
	* \brief Fill a UBO structure with the light's data
	*
	* \param[in] pLight Pointer to the light structure that will be copied into a UBO
	*
	*/
	/*void VELight::fillLightStructure(veLightData_t *pLight) {
		pLight->type[0] = m_lightType;
		pLight->param = param;
		pLight->col_ambient = col_ambient;
		pLight->col_diffuse = col_diffuse;
		pLight->col_specular = col_specular;
		pLight->transform = getWorldTransform();
	}*/

	void VELight::updateUBO(glm::mat4 worldMatrix) {
		veUBOPerLight_t ubo = {};

		ubo.type[0] = getLightType();
		ubo.model = getWorldTransform();
		ubo.col_ambient = m_col_ambient;
		ubo.col_diffuse = m_col_diffuse;
		ubo.col_specular = m_col_specular;
		ubo.param = m_param;

		VESceneObject::updateUBO((void*)&ubo, (uint32_t)sizeof(veUBOPerLight_t));
	}


	VELight::~VELight() {
		for (auto pCam : m_shadowCameras) {
			delete pCam;
		}
		m_shadowCameras.clear();
	};


	//------------------------------------------------------------------------------------------------
	//derive light classes

	VEDirectionalLight::VEDirectionalLight(std::string name, glm::mat4 transf, VESceneNode *parent) :
							VELight(name, transf, parent) {

	};


	VEPointLight::VEPointLight(std::string name, glm::mat4 transf, VESceneNode *parent) :
					VELight( name, transf, parent ) {
	};


	VESpotLight::VESpotLight(std::string name, glm::mat4 transf, VESceneNode *parent) :
					VELight(name, transf, parent) {
	};


}


