#pragma once
#pragma comment(lib,"assimp-vc140-mt.lib")//为了将".lib"文件加入到主程序的链接文件中一起编译


#ifndef MODEL_H
#define MODEL_H
  
#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

class Model
{
public:
	/*  Model Data */
	vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	vector<Mesh> meshes;
	string directory;
	bool gammaCorrection;
	int vNum; // 顶点数量
	string imageName;

	vector<float> xx; // 全部顶点的 X 坐标的vector
	vector<float> yy;
	vector<float> zz;
	float cx, cy, cz, dx, dy;
	float minX, maxX, minY, maxY, minZ, maxZ;

	/*  Functions   */
	// constructor, expects a filepath to a 3D model.
	Model(string const &path, string imgName, int dx, int dy, bool gamma = false) : gammaCorrection(gamma)
	{
		vNum = 0;
		imageName = imgName;
		this->dx = dx;
		this->dy = dy;

		loadModel(path);
	}

	// draws the model, and thus all its meshes
	void Draw(Shader shader)
	{
		for (unsigned int j = 0; j < meshes.size(); j++) {
			meshes[j].Draw(shader);
		}
	}

private:
	void FindMaxMin(vector<float> vec, float &max, float &min) {
		min = max = vec[0];
		for (int i = 1; i < vec.size(); i++) {
			if (vec[i] > max) max = vec[i];
			else if (vec[i] < min) min = vec[i];
		}
	}
	/*  Functions   */
	// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string const &path)
	{
		// 加载模型至数据结构scene中
		// read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('/'));



		//---------------------------------------------------------------------------
		// 对整体而言，得出cx, cy, cz
		fn(scene->mRootNode, scene);
		FindMaxMin(xx, maxX, minX);
		FindMaxMin(yy, maxY, minY);
		FindMaxMin(zz, maxZ, minZ);
		cx = maxX - minX;
		cy = maxY - minY;
		cz = maxZ - minZ;
		//---------------------------------------------------------------------------



		// process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);

	}

	void fn(aiNode *node, const aiScene *scene) {
		for (unsigned int j = 0; j < node->mNumMeshes; j++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[j]];
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				xx.push_back(mesh->mVertices[i].x);
				yy.push_back(mesh->mVertices[i].y);
				zz.push_back(mesh->mVertices[i].z);
			}
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			fn(node->mChildren[i], scene);//递归，处理所有结点
		}
	}

	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode *node, const aiScene *scene)
	{
		// process each mesh located at the current node 处理结点的所有网格
		for (unsigned int i = 0; i < node->mNumMeshes; i++)// mNumMeshes为结点中的网格（索引）数
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			// node->mMeshes[i]为scene中对应网格的索引
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];//索引场景的mMeshes数组来获取对应的网格
			//cout << scene->mMeshes[node->mMeshes[i]]->mName.data << endl; // 得到 mesh 的名字
			meshes.push_back(processMesh(mesh, scene));//processMesh返回一个mesh对象，参数为上面获得的网格
		}
		// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);//递归，处理所有结点
		}

	}

	//访问网格的相关属性并将它们储存到我们自己的对象中
	Mesh processMesh(aiMesh *mesh, const aiScene *scene)
	{
		// data to fill
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		// Walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			vNum++;
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			// normals
			glm::vec3 vn;
			if (i % 3 == 0) {
				glm::vec3 v1, v2, v3;
				v1.x = mesh->mVertices[i].x;
				v1.y = mesh->mVertices[i].y;
				v1.z = mesh->mVertices[i].z;
				v2.x = mesh->mVertices[i + 1].x;
				v2.y = mesh->mVertices[i + 1].y;
				v2.z = mesh->mVertices[i + 1].z;
				v3.x = mesh->mVertices[i + 2].x;
				v3.y = mesh->mVertices[i + 2].y;
				v3.z = mesh->mVertices[i + 2].z;
				vn = normalize(cross(v1 - v2, v1 - v3));
			}
			else if (i % 3 == 1) {
				glm::vec3 v1, v2, v3;
				v1.x = mesh->mVertices[i].x;
				v1.y = mesh->mVertices[i].y;
				v1.z = mesh->mVertices[i].z;
				v2.x = mesh->mVertices[i + 1].x;
				v2.y = mesh->mVertices[i + 1].y;
				v2.z = mesh->mVertices[i + 1].z;
				v3.x = mesh->mVertices[i - 1].x;
				v3.y = mesh->mVertices[i - 1].y;
				v3.z = mesh->mVertices[i - 1].z;
				vn = normalize(cross(v3 - v1, v3 - v2));
			}
			else if (i % 3 == 2) {
				glm::vec3 v1, v2, v3;
				v1.x = mesh->mVertices[i].x;
				v1.y = mesh->mVertices[i].y;
				v1.z = mesh->mVertices[i].z;
				v2.x = mesh->mVertices[i - 1].x;
				v2.y = mesh->mVertices[i - 1].y;
				v2.z = mesh->mVertices[i - 1].z;
				v3.x = mesh->mVertices[i - 2].x;
				v3.y = mesh->mVertices[i - 2].y;
				v3.z = mesh->mVertices[i - 2].z;
				vn = normalize(cross(v1 - v3, v1 - v2));
			}
			vertex.Normal = vn;

			// texture coordinates
			if (mesh->mTextureCoords[0]) // 网格是否存在纹理坐标
			{
				glm::vec2 vec;
				float p_x = (mesh->mVertices[i].x - this->minX) / this->cx;  // 获取纹理坐标（2D图片中的坐标通过顶点坐标对应转换得到）
				float p_y = (mesh->mVertices[i].y - this->minY) / this->cy;

				if (vertex.Normal.z > -0.f) {
				    vec.x = p_x / 2.0f;
					vec.y = ((this->dx - this->dy) / 2.0f + p_y * this->dy) / this->dx;
				}
				else{
					vec.x = (2.0f - p_x) / 2.0f;
					vec.y = ((this->dx - this->dy) / 2.0f + p_y * this->dy) / this->dx;
				}
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			//// tangent
			//vector.x = mesh->mTangents[i].x;
			//vector.y = mesh->mTangents[i].y;
			//vector.z = mesh->mTangents[i].z;
			//vertex.Tangent = vector;
			//// bitangent
			//vector.x = mesh->mBitangents[i].x;
			//vector.y = mesh->mBitangents[i].y;
			//vector.z = mesh->mBitangents[i].z;
			//vertex.Bitangent = vector;

			vertices.push_back(vertex);
		}

		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		// 获取索引
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
				
		}

		// process materials
		//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN
		//// 1. diffuse maps
		//vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//// 2. specular maps
		//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		//// 3. normal maps
		//std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		//textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		//// 4. height maps
		//std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		//textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		// 获取纹理
		string path = this->imageName;
		textures = loadTextures(path, aiTextureType_DIFFUSE, "texture_diffuse");
	    

		//// 调整标准人体模型
		//for (int i = 0; i < vertices.size(); i++) {
		//	//vertices[i].Position.x = vertices[i].Position.x * (9.0f / 10.0f);
		//	//vertices[i].Position.y = vertices[i].Position.y * (cy / cx); // cx, cy为全局变量（整体修改),若cx，cy，cz为局部变量，则为局部修改。（视具体情况和难度分析）
		//	//vertices[i].Position.z = vertices[i].Position.z * (cz / cx);
		//}

		return Mesh(vertices, indices, textures);
	}

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	//vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
	//{
	//	vector<Texture> textures;
	//	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	//	{
	//		aiString str;
	//		mat->GetTexture(type, i, &str);
	//		cout << str.C_Str() << endl;
	//		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
	//		bool skip = false;
	//		for (unsigned int j = 0; j < textures_loaded.size(); j++)
	//		{
	//			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
	//			{
	//				textures.push_back(textures_loaded[j]);
	//				skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
	//				break;
	//			}
	//		}
	//		if (!skip)
	//		{   // if texture hasn't been loaded already, load it
	//			Texture texture;
	//			texture.id = TextureFromFile(str.C_Str(), this->directory);
	//			texture.type = typeName;
	//			texture.path = str.C_Str();
	//			textures.push_back(texture);
	//			textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
	//		}
	//	}
	//	return textures;
	//}

	vector<Texture> loadTextures(string path, aiTextureType type, string typeName) {
		vector<Texture> textures;

		Texture texture;

		texture.id = myTextureFromFile(path, this->directory);
		texture.type = typeName;
		texture.path = path;

		textures.push_back(texture);
		textures_loaded.push_back(texture);
		return textures;
	}

	unsigned int myTextureFromFile(string path, const string &directory)
	{
		string filename = path;
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}

};

#endif
