#include "Draw.h"
#include "Matrix.h"
#include <vector>
#include "ShaderProgram.h"

using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

Draw::Draw()
{
	mapWidth = 0;
	mapHeight = 0;
	win = false;
	Init();
}

void Draw::Init()
{
	string levelFile = "mapdata.txt";
	ifstream infile(levelFile);
	string line;
	bool flag = true;
	while (getline(infile, line) && flag == true) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				flag = false;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[ObjectsLayer]") {
			readEntityData(infile);
		}
	}
}

GLuint Draw::LoadTexture(const char *image)
{
	SDL_Surface *surface = IMG_Load(image);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(surface);
	return textureID;
}

void Draw::DrawMap(ShaderProgram *program)
{
	vector<float> vertexData;
	vector<float> texData;

	glEnable(GL_TEXTURE_2D);
	GLuint mapSheet = LoadTexture("spritesheet.png");
	glBindTexture(GL_TEXTURE_2D, mapSheet);
	//glTranslatef((-tiles * mapWidth * 0.1f), (tiles * mapHeight * 0.5f), 0.0f);

	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % spriteCountX) / (float)spriteCountX;
				float v = (float)(((int)levelData[y][x]) / spriteCountX) / (float)spriteCountY;
				float spriteWidth = 1.0f / (float)spriteCountX;
				float spriteHeight = 1.0f / (float)spriteCountY;
				
				vertexData.insert(vertexData.end(), {
					-2.0, 1.0, -1.0, 3.0, -2.0, 3.0, -2.0, 1.0, -1.0, 1.0, -1.0, 3.0
				});
				texData.insert(texData.end(), { 
					u, v + spriteHeight,
					u + spriteWidth, v,
					u, v,
					u, v + spriteHeight,
					u + spriteWidth, v + spriteHeight,
					u + spriteWidth, v
				});
			}
		}
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

bool Draw::readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new int[mapWidth];
		}
		return true;
	}
}

bool Draw::readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

void Draw::placeEntity(string type, float placeX, float placeY)
{
	if (type == "player")
	{
		cout << "Player";
		float w = 1.0f / 16.0f;
		float h = 1.0f / 16.0f;
		Entity* player = new Entity(placeX, placeY, w, h, type);
		entities.push_back(player);
	}
	if (type == "enemy")
	{
		float w = 1.0f / 16.0f;
		float h = 1.0f / 16.0f;
		Entity* player = new Entity(placeX, placeY, w, h, type);
	}
}

bool Draw::readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str()) / 16 * tiles;
			float placeY = atoi(yPosition.c_str()) / 16 * -tiles;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}
