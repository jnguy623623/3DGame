#pragma once


//--------------------------------------------------------------
//
//  Lunar Lander Game
//  Octree Setup Code Provided by <Kevin M. Smith>
//
//  Author Name:   < Jonathan Nguyen >
//  Date: <Dec 9, 2022>

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"



class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent2(ofDragInfo dragInfo);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void drawAxis(ofVec3f);
		void initLightingAndMaterials();
		void savePicture();
		void toggleWireframeMode();
		void togglePointsDisplay();
		void toggleSelectTerrain();
		void setCameraTarget();
		bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);
		bool raySelectWithOctree(ofVec3f &pointRet);
		glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 p , glm::vec3 n);

		ofCamera *theCam; //the cam
		ofEasyCam cam;	//default provided
		ofCamera trackCam, shipCam;
		vector<ofCamera> cameras;
		int currentCam;

		ofxAssimpModelLoader mars, lander;
		ofLight light;
		Box boundingBox, landerBounds;
		Box testBox;
		vector<Box> colBoxList;
		vector<TreeNode> nodeList;
		bool bLanderSelected = false;
		Octree octree;
		TreeNode selectedNode;
		glm::vec3 mouseDownPos, mouseLastPos;
		bool bInDrag = false;


		ofxIntSlider numLevels;
		ofxPanel gui;

		bool bAltKeyDown;
		bool bCtrlKeyDown;
		bool bWireframe;
		bool bDisplayPoints;
		bool bPointSelected;
		bool bHide;
		bool pointSelected = false;
		bool bDisplayLeafNodes = false;
		bool bDisplayOctree = false;
		bool bDisplayBBoxes = false;
		
		bool bLanderLoaded;
		bool bTerrainSelected;
	
		ofVec3f selectedPoint;
		ofVec3f intersectPoint;

		vector<Box> bboxList;

		const float selectionRange = 4.0;

		//for running time tests
		int startTime;
		int endTime;
		int totalTime;

		Particle player;
		ParticleSystem sys;
		ThrustForce* thrustForceX; //left and right
		ThrustForce* thrustForceY; //up and down
		ThrustForce* thrustForceZ; //fwd and back
		GravityForce* gravityForce; //down only
		TurbulenceForce* turbForce; //for thruster emitter

		ImpulseRadialForce* radialForce;
		ParticleEmitter thrustEmitter; //movement
		ParticleEmitter explodeEmitter; //for crashing

		float fuel;
		float altitude;
		Ray altitudeRadar;
		TreeNode ground;
		float gravity = 1.62; //moon's gravity is 1.62 m/s^2
		float epsilon = 1;
	

		bool bThruster; //used to check if thrusters are on or not
		bool bAltitude;
		bool bGame; //check if game is running or not
		bool bFollow; //is camera following spacecraft?
		bool bLife = true; //intact or combusted?
		bool bInside; //landed inside or not
		bool bStarted; //standby at the beginning


		float closestDistance;
		float collisionVel;
		float speed;
		glm::vec3 vel;
		glm::vec3 norm;
		Box targetArea; //place to land, should be the higher res area on terrain

		ofLight keyLight, fillLight, rimLight; //for landing spot
		ofLight shipLight; //for the ship
		void setupLights();
		void setupCamera();
		void updateCamera();
		void rotateCamera();


		ofSoundPlayer thrustSound;
		ofSoundPlayer explSound;
		bool bExplSound = false;
		ofImage background;
		void spawnLander(glm::vec3 spawnPos);
		void checkCollision();
		void resolveCollision();
		void calculateAltitude();

		ofShader shader;
		ofVbo vbo;
		ofTexture particleTex;
		void loadVbo();

};