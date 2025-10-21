///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/


	// Loads six square BaseColor maps from ambientCG.
	// Files are referenced relative to the exe's working directory (e.g., ...\Debug\textures\).

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/Onyx011_2K-JPG_Color.jpg", "onyx"
	);

	bReturn = CreateGLTexture(
		"textures/Ice002_2K-JPG_Color.jpg", "ice"
	);

	bReturn = CreateGLTexture(
		"textures/Wood066_2K-JPG_Color.jpg", "wood"
	);

	bReturn = CreateGLTexture(
		"textures/Ground035_4K-JPG_Color.jpg", "ground"
	);

	bReturn = CreateGLTexture(
		"textures/Concrete044D_2K-JPG_Color.jpg", "concrete"
	);

	bReturn = CreateGLTexture(
		"textures/Metal049A_2K-JPG_Color.jpg", "metal"
	);

	bReturn = CreateGLTexture(
		"textures/Wood032_2K-JPG_Color.jpg", "wood2"
	);

	bReturn = CreateGLTexture(
		"textures/meat_color_2k.jpg", "meat"
	);



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL plasticClear;
	plasticClear.diffuseColor = glm::vec3(0.95f, 0.95f, 0.95f);  // tint
	plasticClear.specularColor = glm::vec3(0.75f, 0.75f, 0.75f);  // glossy
	plasticClear.shininess = 96.0f;
	plasticClear.tag = "plasticClear";
	m_objectMaterials.push_back(plasticClear);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	// point light 3 - warm orange
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.06f, 0.03f, 0.00f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.95f, 0.50f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 1.0f, 0.9f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 3.8f, 3.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[4].position", -3.2f, 6.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

	m_pShaderManager->setVec3Value("spotLight.ambient", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(42.5f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(48.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();        // countertop
	m_basicMeshes->LoadBoxMesh();          // cutting board
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadCylinderMesh();     // cup body, marinade fill
	m_basicMeshes->LoadTaperedCylinderMesh(); // knife handle, meat chunk
	m_basicMeshes->LoadTorusMesh();        // lid
	m_basicMeshes->LoadSphereMesh();       // pork pieces

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh (counter top)
	scaleXYZ = glm::vec3(24.0f, 1.0f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("onyx");
	SetTextureUVScale(3.0, 2.0);
	SetShaderMaterial("tile");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// set the XYZ scale for the mesh (cutting board)
	scaleXYZ = glm::vec3(8.6f, 0.25f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh (sit on plane: posY = height/2)
	positionXYZ = glm::vec3(-0.5f, 0.125f, 0.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.22f, 0.22f, 0.22f, 1.0f);

	SetShaderTexture("wood");
	SetTextureUVScale(1.6, 1.0);
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale for the mesh (knife blade)
	scaleXYZ = glm::vec3(0.05f, 5.00f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 0.33f, 1.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.20f, 0.55f, 0.90f, 1.0f);
	SetShaderTexture("metal");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("tile");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/


	// set the XYZ scale for the mesh (knife handle)
	scaleXYZ = glm::vec3(0.20f, 1.10f, 0.20f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.15f, 0.30f, 1.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.22f, 0.22f, 0.22f, 1.0f);

	SetShaderTexture("wood2");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// Meat pyramid A
	scaleXYZ = glm::vec3(1.60f, 1.10f, 1.20f);
	XrotationDegrees = -6.0f;
	YrotationDegrees = 18.0f;
	ZrotationDegrees = 4.0f;

	positionXYZ = glm::vec3(-2.25f, 2.00f, 0.60f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderTexture("meat");          
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");

	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Meat pyramid B (slightly different, overlaps A)
	scaleXYZ = glm::vec3(1.20f, 0.90f, 1.00f);
	XrotationDegrees = 8.0f;
	YrotationDegrees = -22.0f;
	ZrotationDegrees = -10.0f;

	positionXYZ = glm::vec3(-1.95f, 1.45f, 1.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");

	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Meat pyramid C — BIG middle piece
	scaleXYZ = glm::vec3(2.75f, 2.75f,2.75f);
	XrotationDegrees = -4.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 2.0f;

	positionXYZ = glm::vec3(-2.30f, 1.45f, 0.70f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees,
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Meat pyramid D
	scaleXYZ = glm::vec3(2.10f, 0.85f, 1.95f);
	XrotationDegrees = 6.0f;
	YrotationDegrees = 28.0f;
	ZrotationDegrees = -8.0f;

	positionXYZ = glm::vec3(-2.95f, 0.60f, 0.35f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees,
		YrotationDegrees, 
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Meat pyramid E
	scaleXYZ = glm::vec3(1.95f, 0.75f, 1.90f);
	XrotationDegrees = -38.0f;
	YrotationDegrees = -48.0f;
	ZrotationDegrees = 12.0f;

	positionXYZ = glm::vec3(-1.70f, 1.10f, 0.35f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// set the XYZ scale for the mesh (marinade)
	scaleXYZ = glm::vec3(1.0f, 2.65f, 1.00f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.2f, 2.75f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.30f, 0.12f, 0.08f, 1.0f);

	SetShaderMaterial("tile");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// Pork piece A (pyramid) — peeking out near rim
	scaleXYZ = glm::vec3(0.34f, 0.26f, 0.28f);
	XrotationDegrees = 12.0f;
	YrotationDegrees = 18.0f;
	ZrotationDegrees = -8.0f;

	positionXYZ = glm::vec3(5.18f, 2.75f, -1.06f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Pork piece B (pyramid) — offset to the back-right
	scaleXYZ = glm::vec3(0.28f, 0.24f, 0.26f);
	XrotationDegrees = -6.0f; 
	YrotationDegrees = 32.0f; 
	ZrotationDegrees = 10.0f;

	positionXYZ = glm::vec3(5.32f, 2.75f, -0.88f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Pork piece C (pyramid) — smaller front-left nub
	scaleXYZ = glm::vec3(0.54f, 0.50f, 0.52f);
	XrotationDegrees = 8.0f; 
	YrotationDegrees = -20.0f; 
	ZrotationDegrees = -12.0f;

	positionXYZ = glm::vec3(5.06f, 2.75f, -1.14f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Pork piece D (pyramid)
	scaleXYZ = glm::vec3(0.35f, 0.27f, 0.29f);
	XrotationDegrees = 12.0f;
	YrotationDegrees = 18.0f;
	ZrotationDegrees = -8.0f;

	positionXYZ = glm::vec3(5.18f, 2.75f, -1.36f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Pork piece E (pyramid)
	scaleXYZ = glm::vec3(0.48f, 0.44f, 0.46f);
	XrotationDegrees = -6.0f;
	YrotationDegrees = 32.0f;
	ZrotationDegrees = 10.0f;

	positionXYZ = glm::vec3(5.32f, 2.75f, -0.68f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// Pork piece F (pyramid)
	scaleXYZ = glm::vec3(0.34f, 0.30f, 0.32f);
	XrotationDegrees = 8.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = -12.0f;

	positionXYZ = glm::vec3(5.06f, 2.75f, -1.84f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("meat");
	SetTextureUVScale(1.5f, 1.2f);
	SetShaderMaterial("clay");
	m_basicMeshes->DrawPyramid4Mesh();
	/****************************************************************/

	// set the XYZ scale for the mesh (cup body)
	scaleXYZ = glm::vec3(1.05f, 3.0f, 1.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.2f, 3.0f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.35f);

	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	// set the XYZ scale for the mesh (lid)
	scaleXYZ = glm::vec3(1.10f, 0.10f, 1.10f);
	XrotationDegrees = 0.0f;  YrotationDegrees = 0.0f;  ZrotationDegrees = 0.0f;
	// y = half the lid height so it rests on the plane (y=0)
	positionXYZ = glm::vec3(6.8f, 0.05f, 0.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// see through plastic
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.35f);
	SetShaderMaterial("plasticClear");

	// thin cylinder with caps
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// Tweezers — left strip 
	scaleXYZ = glm::vec3(1.80f, 0.05f, 0.02f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 9.5f;                   
	ZrotationDegrees = 2.0f;

	positionXYZ = glm::vec3(1.46f, 0.3f, 1.855f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees,
		positionXYZ
	);
	SetShaderTexture("metal"); 
	SetTextureUVScale(2.0, 1.0); 
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// Tweezers — right strip
	scaleXYZ = glm::vec3(1.80f, 0.05f, 0.02f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 12.0f;
	ZrotationDegrees = 2.0f;

	positionXYZ = glm::vec3(1.48f, 0.30f, 1.915f);
	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ
	);

	SetShaderTexture("metal");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

}