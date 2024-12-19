///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/


	this->CreateGLTexture("textures/Wood.jpg", "Wood");
	this->CreateGLTexture("textures/mac.jpg", "laptop");
	this->CreateGLTexture("textures/white.jpg", "white");
	this->CreateGLTexture("textures/jotter.png", "jotter");
	this->CreateGLTexture("textures/pod.jpg", "Pod");
	this->CreateGLTexture("textures/pen.png", "pen");
	this->CreateGLTexture("textures/rubber.jpg", "rubber");
	this->CreateGLTexture("textures/glass.jpg", "glass");
	this->CreateGLTexture("textures/base.jpg", "base");
	this->CreateGLTexture("textures/case.png", "case");
	


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

	//Define Objects Materials To improve lignting effect


	// Normal Material
	OBJECT_MATERIAL normal_Material_shade;
	normal_Material_shade.ambientColor = glm::vec3(0.02f, 0.02f, 0.02f); // Uniform gray
	normal_Material_shade.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);    // Balanced diffuse for visibility
	normal_Material_shade.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);   // Low specular for matte look
	normal_Material_shade.shininess = 32.0f;                             // Moderate shininess
	normal_Material_shade.ambientStrength = 0.2f;                       // Stronger ambient strength
	normal_Material_shade.tag = "NORMAL";

	// Glass Material
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);            // Minimal ambient for glass
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);            // Subtle diffuse for edges
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);           // Strong specular for shine
	glassMaterial.shininess = 128.0f;                                    // Very shiny surface
	glassMaterial.ambientStrength = 0.1f;                               // Light ambient for clarity
	glassMaterial.tag = "GlassMaterial";

	// Water Material
	OBJECT_MATERIAL waterMaterial;
	waterMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.4f);            // Slight blue tint for water
	waterMaterial.diffuseColor = glm::vec3(0.3f, 0.5f, 0.8f);            // Softer diffuse for transparency
	waterMaterial.specularColor = glm::vec3(0.6f, 0.8f, 1.0f);           // High specular for reflection
	waterMaterial.shininess = 64.0f;                                     // Moderate shininess
	waterMaterial.ambientStrength = 0.25f;                              // Balanced ambient
	waterMaterial.tag = "WaterMaterial";

	// Rubber Material
	OBJECT_MATERIAL rubberMaterial;
	rubberMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);           // Low ambient for rubber
	rubberMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);           // Visible diffuse light
	rubberMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);          // Minimal specular for matte finish
	rubberMaterial.shininess = 8.0f;                                     // Low shininess for a rubber-like effect
	rubberMaterial.ambientStrength = 0.15f;                             // Balanced ambient strength
	rubberMaterial.tag = "RubberMaterial";

	// Plastic Material
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);   // Slight ambient for smooth appearance
	plasticMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);   // Strong diffuse for smooth light scattering
	plasticMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Moderate specular for semi-glossy effect
	plasticMaterial.shininess = 32.0f;                            // Medium shininess for a plastic-like reflection
	plasticMaterial.ambientStrength = 0.1f;                      // Moderate ambient strength
	plasticMaterial.tag = "PlasticMaterial";
	m_objectMaterials.push_back(plasticMaterial);

	// Aluminum Material
	OBJECT_MATERIAL aluminumMaterial;
	aluminumMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);  // Subtle base reflection
	aluminumMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Brighter diffuse for metal shine
	aluminumMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.6f); // High specular for a metallic look
	aluminumMaterial.shininess = 64.0f;                           // Medium shininess
	aluminumMaterial.ambientStrength = 0.5f;                     // Enhanced ambient light reflection
	aluminumMaterial.tag = "AluminumMaterial";
	m_objectMaterials.push_back(aluminumMaterial);



	// Push all materials to the list
	m_objectMaterials.push_back(normal_Material_shade);
	m_objectMaterials.push_back(glassMaterial);
	m_objectMaterials.push_back(waterMaterial);
	m_objectMaterials.push_back(rubberMaterial);





	m_objectMaterials.push_back(normal_Material_shade);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Light Source 1: Warm Light (Left Side of the Scene)
	m_pShaderManager->setBoolValue("lightSources[0].bActive", true);
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(-10.0f, 5.0f, 0.0f)); // Positioned to the left
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.05f, 0.05f, 0.05f)); // Soft ambient
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.25f, 0.2f, 0.15f));  // Warm diffuse
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.25f, 0.225f, 0.2f)); // Warm specular

	// Light Source 2: Cool Light (Left Side, Further Back)
	m_pShaderManager->setBoolValue("lightSources[1].bActive", true);
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-10.0f, 8.0f, -5.0f)); // Left and back
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.025f, 0.025f, 0.05f)); // Cool ambient
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.15f, 0.175f, 0.25f));  // Cool diffuse
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.125f, 0.15f, 0.2f));  // Cool specular

	// Light Source 3: Overhead Light (Centered but Slightly Left)
	m_pShaderManager->setBoolValue("lightSources[2].bActive", true);
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(-5.0f, 12.0f, 0.0f)); // Centered overhead but shifted left
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.075f, 0.075f, 0.075f)); // Neutral ambient
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.25f, 0.25f, 0.25f));    // Neutral diffuse
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.25f, 0.25f, 0.25f));   // Neutral specular

	// Disable any additional unused lights
	for (int i = 3; i < 4; ++i)
	{
		std::stringstream ss;
		ss << "lightSources[" << i << "].bActive";
		m_pShaderManager->setBoolValue(ss.str().c_str(), false);
	}
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

	LoadSceneTextures();

	//load Box mesh For Rendering Books
	m_basicMeshes->LoadBoxMesh();
	//load Plane Mesh For rendering the Grpound
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();

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
	//table
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(35.50f, 9.0f, 15.50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -10.0f, 10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("base");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/




	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 13.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("Wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	//legs
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.50f, 9.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.0f, -4.0f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("Wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/



	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.50f, 9.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, -4.0f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("Wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/



	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.50f, 9.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.0f, -4.0f, 15.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("Wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.50f, 9.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, -4.0f, 15.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 1);

	SetShaderMaterial("NORMAL");

	SetShaderTexture("Wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/



	// Draw MacBook
/******************************************************************/
// Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.250f, 0.050f, 2.250f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.5f, 8.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the aluminum material for the MacBook
	SetShaderMaterial("AluminumMaterial");

	// Optionally apply a texture for fine details (e.g., brushed metal look)
	SetShaderTexture("laptop");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

		//draw Air Pods
			/******************************************************************/
		// set the XYZ scale for the mesh
		//first Book Draw One Cube For the Outer Layer
		scaleXYZ = glm::vec3(0.080f);

		// set the XYZ rotation for the mesh
		// Render First AirPod
			// Bud
				// Render First AirPod
			// Bud
		// First AirPod
		// Bud
		scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);
		positionXYZ = glm::vec3(3.0f, 0.66f, 8.0f); // Slightly adjusted above the table
		XrotationDegrees = -5.0f;
		YrotationDegrees = 2.0f;
		ZrotationDegrees = -3.0f;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("PlasticMaterial"); // Updated to plastic material
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White plastic
		SetShaderTexture("Pod");
		m_basicMeshes->DrawSphereMesh();

		// Ear Tip
		scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);
		positionXYZ = glm::vec3(3.02f, 0.64f, 8.1f); // Adjusted slightly forward
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("RubberMaterial"); // Rubber texture for the ear tip
		SetShaderTexture("rubber");
		m_basicMeshes->DrawSphereMesh();

		// Stem
		scaleXYZ = glm::vec3(0.05f, 0.4f, 0.05f);
		positionXYZ = glm::vec3(3.0f, 0.58f, 7.92f); // Adjusted to align with the bud
		XrotationDegrees = 92.0f; // Vertical orientation with a slight tilt
		YrotationDegrees = 5.0f;
		ZrotationDegrees = -2.0f;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("PlasticMaterial");
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White plastic
		SetShaderTexture("Pod");
		m_basicMeshes->DrawCylinderMesh();

		// Second AirPod
		// Bud
		scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);
		positionXYZ = glm::vec3(2.6f, 0.65f, 8.05f); // Slightly adjusted for a scattered look
		XrotationDegrees = 25.0f;
		YrotationDegrees = -25.0f;
		ZrotationDegrees = 15.0f;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("PlasticMaterial");
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White plastic
		SetShaderTexture("Pod");
		m_basicMeshes->DrawSphereMesh();

		// Ear Tip
		scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);
		positionXYZ = glm::vec3(2.62f, 0.63f, 8.15f); // Adjusted slightly forward
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("RubberMaterial");
		SetShaderTexture("rubber");
		m_basicMeshes->DrawSphereMesh();

		// Stem
		scaleXYZ = glm::vec3(0.05f, 0.4f, 0.05f);
		positionXYZ = glm::vec3(2.58f, 0.58f, 8.0f); // Slight adjustment for natural placement
		XrotationDegrees = 95.0f; // Angled orientation
		YrotationDegrees = -30.0f;
		ZrotationDegrees = 10.0f;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("PlasticMaterial");
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White plastic
		SetShaderTexture("Pod");
		m_basicMeshes->DrawCylinderMesh();





	//Draw Cup
	/******************************************************************/
	// set the XYZ scale for the mesh
	//first Book Draw One Cube For the Outer Layer
	// Draw Outer Cylinder (Outer Glass Body)

	// Base position for the cup
	// Table Top Position
	float tableTopY = -0.1f;  // Height of the table's surface
	float glassHeight = 2.17f; // Height of the glass

	// Base position of the cup (to align with the table surface)
	glm::vec3 cupBasePosition = glm::vec3(2.44f, tableTopY + glassHeight / 2.0f, 6.0f);

	// **************************
	// Draw Glass Outer Cylinder
	// **************************
	glm::vec3 glassOuterPosition = cupBasePosition; // Center of the glass
	scaleXYZ = glm::vec3(0.503f, glassHeight / 2.0f, 0.503f); // Outer dimensions
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glassOuterPosition);

	SetShaderMaterial("GlassMaterial");          // Transparent glass material
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f);      // White color with transparency
	m_basicMeshes->DrawCylinderMesh();           // Default cylinder mesh

	// **************************
	// Draw Glass Inner Cylinder
	// **************************
	glm::vec3 glassInnerPosition = glassOuterPosition; // Same position as outer glass
	scaleXYZ = glm::vec3(0.45f, glassHeight / 2.0f - 0.025f, 0.45f); // Slightly shorter inner cylinder
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glassInnerPosition);

	SetShaderMaterial("GlassMaterial");
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f);
	m_basicMeshes->DrawCylinderMesh();

	// **************************
	// Draw Bottom
	// **************************
	float waterHeight = glassHeight * 0.75f; // 3/4th of the glass height
	glm::vec3 waterPosition = glm::vec3(cupBasePosition.x, tableTopY + waterHeight / 2.0f, cupBasePosition.z); // Centered water inside the glass
	scaleXYZ = glm::vec3(0.45f, waterHeight / 2.0f, 0.45f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, waterPosition);

	SetShaderMaterial("WaterMaterial");         // Transparent blue material for water
	m_basicMeshes->DrawCylinderMesh();






	/****************************************************************/


	//Planner
	/******************************************************************/
	// set the XYZ scale for the mesh
	//first Book Draw One Cube For the Outer Layer
	// Combined Books
/******************************************************************/
// Set the XYZ scale for the mesh (twice the width to represent two books side by side)
	scaleXYZ = glm::vec3(3.0f, 0.01f, 1.50f);  // Wider scale to include both books

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh (centered between the positions of the original books)
	positionXYZ = glm::vec3(-0.28f, 0.5f, 11.1f); // Adjust position to account for both books

	// Set the transformations into memory to be used on the drawn mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply a single texture
	SetShaderMaterial("NORMAL"); // Use a default or book-specific material
	SetShaderTexture("jotter");  // The texture applied to both books

	// Draw the combined mesh
	m_basicMeshes->DrawBoxMesh();



	/****************************************************************/


	// Draw the Pen Body
/******************************************************************/
// Set the XYZ scale for the pen body
	scaleXYZ = glm::vec3(0.05f, 0.9f, 0.05f); // Slim and long for the body

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;  // Lay horizontally
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f; // Reverse orientation

	// Set the XYZ position for the pen body
	positionXYZ = glm::vec3(0.44f, 0.5f, 11.19f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the pen body material and texture
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);  // Dark black for the pen body
	SetShaderMaterial("PlasticMaterial");    // Plastic for the pen body
	SetShaderTexture("pen");                 // Apply texture for detailing

	// Draw the pen body (tapered cylinder for slight variation)
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Draw the Pen Tip
	/******************************************************************/
	// Set the XYZ scale for the pen tip
	scaleXYZ = glm::vec3(0.03f, 0.1f, 0.03f); // Small and sharp for the tip

	// Set the XYZ position for the pen tip (aligned to the body)
	positionXYZ = glm::vec3(0.44f, 0.49f, 11.0f); // Slightly in front of the body

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the tip material and texture
	SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);  // Metallic silver for the tip
	SetShaderMaterial("AluminumMaterial");      // Metallic material for the tip
	SetShaderTexture("pen");

	// Draw the pen tip
	m_basicMeshes->DrawConeMesh(); // Cone shape for the tip

	// Draw the Pen Grip
	/******************************************************************/
	// Set the XYZ scale for the pen grip
	scaleXYZ = glm::vec3(0.055f, 0.15f, 0.055f); // Slightly thicker than the body

	// Set the XYZ position for the pen grip (near the tip)
	positionXYZ = glm::vec3(0.44f, 0.49f, 11.09f); // Just behind the tip

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the grip material and texture
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark rubber for the grip
	SetShaderMaterial("RubberMaterial");     // Rubber material for the grip
	SetShaderTexture("rubber");

	// Draw the pen grip
	m_basicMeshes->DrawCylinderMesh(); // Simple cylinder for the grip

	// Draw the Pen Cap
	/******************************************************************/
	// Set the XYZ scale for the pen cap
	scaleXYZ = glm::vec3(0.055f, 0.3f, 0.055f); // Slightly larger than the body

	// Set the XYZ position for the pen cap (aligned to the opposite end)
	positionXYZ = glm::vec3(0.44f, 0.51f, 11.38f); // On the opposite end of the body

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the cap material and texture
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);  // Dark plastic for the cap
	SetShaderMaterial("PlasticMaterial");    // Plastic material for the cap
	SetShaderTexture("pen");

	// Draw the pen cap
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/


	// Base position for the AirPods case (ensures it is on the table)
	glm::vec3 caseBasePosition = glm::vec3(3.5f, 0.65f, 9.2f);

	// Dimensions for the case
	float caseWidth = 0.90f;  // Width of the case
	float caseHeight = 0.50f; // Height of the case
	float caseDepth = 0.15f;  // Depth of the case

	// Define scaling factors for the sphere
	glm::vec3 sphereScale = glm::vec3(caseWidth / 2.0f, caseHeight / 2.0f, caseDepth / 2.0f);

	// Create the transformation matrix
	glm::mat4 modelMatrix = glm::mat4(1.0f); // Initialize to identity matrix

	// Apply transformations in the correct order:
	// 1. Translate to the base position (centered horizontally on the table)
	modelMatrix = glm::translate(modelMatrix, caseBasePosition);

	// 2. Rotate 90 degrees around the X-axis to lay the shape horizontally
	modelMatrix = glm::rotate(modelMatrix, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	// 3. Apply scaling to create the desired dimensions
	modelMatrix = glm::scale(modelMatrix, sphereScale);

	// Send the matrix to the shader
	m_pShaderManager->setMat4Value("model", modelMatrix);

	// Apply the corrected texture orientation
	SetTextureUVScale(-1.0f, 1.0f); // Flip texture to fix the upside-down issue

	// Render the sphere
	SetShaderMaterial("PlasticMaterial"); // Plastic material
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f); // Light gray plastic
	SetShaderTexture("case");
	m_basicMeshes->DrawSphereMesh();





}
