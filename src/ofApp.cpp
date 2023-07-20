
//--------------------------------------------------------------
//
//  Lunar Lander Game
//  Octree Setup Code Provided by <Kevin M. Smith>
//
//  Author Name:   < Jonathan Nguyen >
//  Date: <Dec 9, 2022>

#include "ofApp.h"
#include "Util.h"
#include <glm/gtx/intersect.hpp>


//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup(){
	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bLanderLoaded = false;
	bTerrainSelected = true;
//	ofSetWindowShape(1024, 768);


	
	cam.setNearClip(.1);
	cam.setFov(65.5);   // approx equivalent to 28mm in 35mm format
	ofSetVerticalSync(true);
	ofEnableSmoothing();
	ofEnableDepthTest();

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();

	//set up sounds and images
	if (!explSound.load("sounds/Explosion1.ogg")) {
		cout << "Can't load sound" << endl;
		ofExit();
	}

	if (!thrustSound.load("sounds/thruster.mp3")) {
		cout << "Can't load sound" << endl;
		ofExit();
	}

	if (!background.load("images/starfield.jpg")) {
		cout << "Can't load image" << endl;
		ofExit();
	}

	mars.loadModel("geo/moon-houdini.obj"); 
	//not actually mars, but I'm too lazy to replace the variable name
	mars.setScaleNormalization(false);

	// create sliders for octree
	//
	gui.setup();
	gui.add(numLevels.setup("Number of Octree Levels", 1, 1, 10));
	bHide = true;

	//  Create Octree
	octree.create(mars.getMesh(0), 20);
	
	testBox = Box(Vector3(3, 3, 0), Vector3(5, 5, 2));


	//camera setup
	setupCamera();
	theCam = &cam; //start as easyCam
	cameras.push_back(cam);
	cameras.push_back(shipCam);
	cameras.push_back(trackCam);
	
	bFollow = true;


	//Forces, thrusts have no initial force
	gravityForce = new GravityForce(ofVec3f(0, -gravity, 0)); //moon's gravity
	thrustForceX = new ThrustForce(ofVec3f(0, 0, 0));
	thrustForceY = new ThrustForce(ofVec3f(0, 0, 0));
	thrustForceZ = new ThrustForce(ofVec3f(0, 0, 0));

	//forces for emitters
	radialForce = new ImpulseRadialForce(1000.0);
	turbForce = new TurbulenceForce(glm::vec3(-90, -90, -90),
		glm::vec3(90, 90, 90));

	player.mass = 50;
	fuel = 2500;

	targetArea = Box(Vector3(-24.6, -1, 5. - 24.6), Vector3(24.6, 1.5, 24.6));

	//spawn lander here
	spawnLander(glm::vec3(0, 50, 0));
	sys.add(player);
	sys.addForce(gravityForce);
	sys.addForce(thrustForceX);
	sys.addForce(thrustForceY);
	sys.addForce(thrustForceZ);

	explodeEmitter.sys->addForce(radialForce);
	explodeEmitter.setVelocity(ofVec3f(0, 0, 0));
	explodeEmitter.setOneShot(true);
	explodeEmitter.setEmitterType(RadialEmitter);
	explodeEmitter.setGroupSize(500);

	thrustEmitter.velocity = glm::vec3(0, -15, 0);
	thrustEmitter.rate = 70;
	thrustEmitter.setEmitterType(DirectionalEmitter);
	thrustEmitter.setGroupSize(70);
	thrustEmitter.lifespan = 0.15;
	thrustEmitter.setParticleRadius(0.05);
	thrustEmitter.sys->addForce(turbForce);


	//setupLights(); /set up landing position lights and spacecraft lights
	bStarted = false;
	//bInside = true;
	bAltitude = true;
	bGame = false;
	bLife = true;

}
 
//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {


	//Only move the craft when game is running and set positions
	// If game is not running, we can drag the lander around
	//
	if (bGame == true) {
		sys.update();

		if (sys.particles.size() > 0) {
			player.position = sys.particles[0].position;
		}
		

		lander.setPosition(player.position.x, player.position.y, player.position.z);
		thrustEmitter.setPosition(lander.getPosition());
		shipLight.setPosition(player.position);
	}

	
	for (auto& p : explodeEmitter.sys->particles) {
		p.position.set(ofVec3f(player.position));
	}


	
	//if (explodeEmitter.started) {
		explodeEmitter.update();
	//}
	//if (thrustEmitter.started) {
		thrustEmitter.update();
	//}
	
	//explodeEmitter.update();
	//thrustEmitter.update();

	updateCamera();
	checkCollision();
	calculateAltitude();

	// consume fuel
	if (bThruster == true && fuel >= 0 && bGame == true) {
		fuel--;
		thrustEmitter.start();


		//also play thruster sound here
		if (!thrustSound.getIsPlaying()) {
			thrustSound.play();
		}
		
	}
	else {
		thrustEmitter.stop();
		thrustSound.stop();
	}
	
		
	
}
//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(ofColor::black);
	
	//background
	ofDisableDepthTest();
	ofSetColor(ofColor::white);
	background.draw(0, 0, 0);
	ofEnableDepthTest();

	//gui
	glDepthMask(false);
	if (!bHide) gui.draw();
	glDepthMask(true);

	loadVbo();

	theCam->begin(); 
	ofPushMatrix();

	//do lighting here


	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		mars.drawWireframe();
		if (bLanderLoaded) {
			lander.drawWireframe();
			if (!bTerrainSelected) drawAxis(lander.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();              // shaded mode
		mars.drawFaces();
		ofMesh mesh;
		if (bLanderLoaded) {
			lander.drawFaces();
			if (!bTerrainSelected) drawAxis(lander.getPosition());
			if (bDisplayBBoxes) {
				ofNoFill();
				ofSetColor(ofColor::white);
				for (int i = 0; i < lander.getNumMeshes(); i++) {
					ofPushMatrix();
					ofMultMatrix(lander.getModelMatrix());
					ofRotate(-90, 1, 0, 0);
					Octree::drawBox(bboxList[i]);
					ofPopMatrix();
				}
			}

			if (bLanderSelected) {

				ofVec3f min = lander.getSceneMin() + lander.getPosition();
				ofVec3f max = lander.getSceneMax() + lander.getPosition();

				Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
				ofSetColor(ofColor::white);
				Octree::drawBox(bounds);

				// draw colliding boxes
				//
				ofSetColor(ofColor::lightBlue);
				for (int i = 0; i < colBoxList.size(); i++) {
					Octree::drawBox(colBoxList[i]);
				}
			}
		}
	}
	if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));



	if (bDisplayPoints) {                // display points as an option    
		glPointSize(3);
		ofSetColor(ofColor::green);
		mars.drawVertices();
	}

	// highlight selected point (draw sphere around selected point)
	//
	if (bPointSelected) {
		ofSetColor(ofColor::blue);
		ofDrawSphere(selectedPoint, .1);
	}

	//shader
	glDepthMask(GL_FALSE);
	ofSetColor(ofColor::red);
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnablePointSprites();
	shader.begin();
	particleTex.bind();
	vbo.draw(GL_POINTS, 0, (int)thrustEmitter.sys->particles.size());
	particleTex.unbind();
	shader.end();
	ofDisablePointSprites();
	ofDisableBlendMode();
	ofEnableAlphaBlending();
	glDepthMask(GL_TRUE);


	// recursively draw octree
	//
	ofDisableLighting();
	int level = 0;
	//	ofNoFill();

	if (bDisplayLeafNodes) {
		octree.drawLeafNodes(octree.root);
		cout << "num leaf: " << octree.numLeaf << endl;
    }
	else if (bDisplayOctree) {
		ofNoFill();
		ofSetColor(ofColor::white);
		octree.draw(numLevels, 0);
	}

	// if point selected, draw a sphere
	//
	if (pointSelected) {
		
		ofVec3f p = octree.mesh.getVertex(selectedNode.points[0]);
		ofVec3f d = p - cam.getPosition();
		ofSetColor(ofColor::lightGreen);
		//ofSetColor(ofColor::purple); //changed colors for testing
		ofDrawSphere(p, .02 * d.length());
	}



	//lights
	//ofEnableLighting();
	//ofSetColor(ofColor::white);
	//keyLight.draw();
	//fillLight.draw();
	//rimLight.draw();
	//shipLight.draw();

	ofPopMatrix();
	ofSetColor(ofColor::red);

	//if(explodeEmitter.started)
	explodeEmitter.draw();

	//if (thrustEmitter.started)
	thrustEmitter.draw();

	//easyCam.end();				//easyCam?
	theCam->end();
	ofDisableDepthTest();
	ofDrawBitmapString("FUEL: " + ofToString(fuel), 0, 125, 0);
	if(bAltitude)
		ofDrawBitmapString("AGL: " + ofToString(altitude), 0, 150, 0);

	if (currentCam == 0) {
		ofDrawBitmapString("CAMERA: EASY CAM", 0, 100, 0);
	}
	else if (currentCam == 1) {
		ofDrawBitmapString("CAMERA: SHIP CAM", 0, 100, 0);
	}
	else if (currentCam == 2) {
		ofDrawBitmapString("CAMERA: TRACK CAM", 0, 100, 0);
	}

	if (bLife == false) {
		ofDrawBitmapString("STATUS: --CRASHED--", 0, 75, 0);
		ofDrawBitmapString("CRASHED - MISSION FAILED",
			ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2, 0);
	}
	else {
		ofDrawBitmapString("STATUS: --OPERATIONAL--", 0, 75, 0);
	}

	if (bStarted == false) {
		ofDrawBitmapString("STANDBY - PRESS [SPACE BAR] TO START",
			ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2, 0);
	}

	if (bInside == true && bGame == false && bStarted == true && bLife == true) {
		ofDrawBitmapString("LANDED INSIDE - MISSON ACCOMPLISHED",
			ofGetWindowWidth()/2 - 100, ofGetWindowHeight()/2, 0);
			//0, 0, 0);
	}
	else if ( bInside == false && bGame == false && bStarted == true && bLife == true) {
		ofDrawBitmapString("LANDED OUTSIDE - MISSION FAILED",
			ofGetWindowWidth()/2 - 100, ofGetWindowHeight()/2 , 0);
			//0, 0, 0);
	}

	ofEnableDepthTest();
}


// 
// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));
	

	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}


void ofApp::keyPressed(int key) {

	switch (key) {
	case 'A':
	case 'a':
		bThruster = true;
		player.angularVel = -30;
		break;
	case 'B':
	case 'b':
		bDisplayBBoxes = !bDisplayBBoxes;
		break;
	case 'C':
	case 'c':
		if (cam.getMouseInputEnabled()) cam.disableMouseInput();
		else cam.enableMouseInput();
		break;
	case 'D':
	case 'd':
		bThruster = true;
		player.angularVel = 30;
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
	case 'H'://I guess we toggle altitude display here
	case 'h':
		bAltitude = !bAltitude;
		break;
	case 'L':
	case 'l':
		bDisplayLeafNodes = !bDisplayLeafNodes;
		break;
	case 'O':
	case 'o':
		bDisplayOctree = !bDisplayOctree;
		break;
	case 'r':
		cam.reset();
		break;
	case 's':
		savePicture();
		break;
	case 't':
		setCameraTarget();
		break;
	case 'U':
	case 'u':
		//rotateCamera();
		break;
	case 'v':
		togglePointsDisplay();
		break;
	case 'V':
		break;
	case 'w':
		toggleWireframeMode();
		break;
	case 'X':
	case 'x':
		thrustForceY->setForce(ofVec3f(0, -100, 0));
		bThruster = true;
		break;
	case 'Y':
	case 'y':
		bFollow = !bFollow; //toggle easyCam follow
		break;
	case 'Z':
	case 'z':
		thrustForceY->setForce(ofVec3f(0, 100, 0));
		bThruster = true;
		break;
	case OF_KEY_UP:
		//code
		thrustForceZ->setForce(ofVec3f(0, 0, 100));
		bThruster = true;
		break;
	case OF_KEY_DOWN:
		//code
		thrustForceZ->setForce(ofVec3f(0, 0, -100));
		bThruster = true;
		break;
	case OF_KEY_LEFT:
		//code
		thrustForceX->setForce(ofVec3f(-100, 0, 0));
		bThruster = true;
		break;
	case OF_KEY_RIGHT:
		//code
		thrustForceX->setForce(ofVec3f(100, 0, 0));
		bThruster = true;
		break;
	case OF_KEY_ALT:
		cam.enableMouseInput();
		bAltKeyDown = true;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = true;
		break;
	case OF_KEY_SHIFT:
		break;
	case OF_KEY_DEL:
		break;
	case ' ':
		bStarted = true;
		bGame = true;
		break;
	case '1':
		theCam = &cam;
		currentCam = 0;
		break;
	case '2':
		theCam = &shipCam;
		currentCam = 1;
		break;
	case '3':
		theCam = &trackCam;
		currentCam = 2;
		break;
	default:
		break;
	}

}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

void ofApp::keyReleased(int key) {

	switch (key) {
	case 'A':
	case 'a':
		bThruster = false;
		player.angularVel = 0;
		break;
	case 'D':
	case 'd':
		bThruster = false;
		player.angularVel = 0;
		break;
	case 'X':
	case 'x':
		thrustForceY->setForce(ofVec3f(0, 0, 0));
		bThruster = false;
		break;
	case 'Z':
	case 'z':
		thrustForceY->setForce(ofVec3f(0, 0, 0));
		bThruster = false;
		break;
	case OF_KEY_UP:
		//code
		thrustForceZ->setForce(ofVec3f(0,0,0));
		bThruster = false;
		break;
	case OF_KEY_DOWN:
		//code
		thrustForceZ->setForce(ofVec3f(0, 0, 0));
		bThruster = false;
		break;
	case OF_KEY_LEFT:
		//code
		thrustForceX->setForce(ofVec3f(0, 0, 0));
		bThruster = false;
		break;
	case OF_KEY_RIGHT:
		//code
		thrustForceX->setForce(ofVec3f(0, 0, 0));
		bThruster = false;
		break;
	case OF_KEY_ALT:
		cam.disableMouseInput();
		bAltKeyDown = false;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = false;
		break;
	case OF_KEY_SHIFT:
		break;
	default:
		break;

	}
}



//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	// if moving camera, don't allow mouse interaction
//
	if (cam.getMouseInputEnabled()) return;

	// if rover is loaded, test for selection
	//
	if (bLanderLoaded) {
		glm::vec3 origin = cam.getPosition();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = lander.getSceneMin() + lander.getPosition();
		ofVec3f max = lander.getSceneMax() + lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			bLanderSelected = true;
			mouseDownPos = getMousePointOnPlane(lander.getPosition(), cam.getZAxis());
			mouseLastPos = mouseDownPos;
			if (bGame == false) {
				bInDrag = true; //only enable dragging when game is not running
			}
				
		}
		else {
			bLanderSelected = false;
		}
	}
	else {
		//start timer here
		startTime = ofGetElapsedTimeMicros();
		ofVec3f p;
		raySelectWithOctree(p);
		endTime = ofGetElapsedTimeMicros();
		totalTime = endTime - startTime;
		//cout << "Time:" << totalTime << " microseconds" << endl;
	}
}

bool ofApp::raySelectWithOctree(ofVec3f &pointRet) {
	ofVec3f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));

	pointSelected = octree.intersect(ray, octree.root, selectedNode);

	if (pointSelected) {
		
		pointRet = octree.mesh.getVertex(selectedNode.points[0]);

	}
	return pointSelected;
}




//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

		if (bInDrag) {

			glm::vec3 landerPos = lander.getPosition();

			glm::vec3 mousePos = getMousePointOnPlane(landerPos, cam.getZAxis());
			glm::vec3 delta = mousePos - mouseLastPos;

			landerPos += delta;
			lander.setPosition(landerPos.x, landerPos.y, landerPos.z);
			player.position = lander.getPosition();
			mouseLastPos = mousePos;

			ofVec3f min = lander.getSceneMin() + lander.getPosition();
			ofVec3f max = lander.getSceneMax() + lander.getPosition();

			Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

			colBoxList.clear();
			octree.intersect(bounds, octree.root, colBoxList);
		}


		else {
			ofVec3f p;
			raySelectWithOctree(p);
		}
	
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}



void ofApp::spawnLander(glm::vec3 spawnPos) {

	//if(lander.loadModel("geo/lander.obj")){ //tells to load the model
		lander.loadModel("geo/lander.obj");
		lander.setScaleNormalization(false); //do not normalize

		player.position = ofVec3f(spawnPos.x, spawnPos.y, spawnPos.z); //sets position
		lander.setPosition(player.position.x, player.position.y, player.position.z);
		bLanderLoaded = true;

		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		// Now position the lander's origin at that intersection point
			//
		glm::vec3 min = lander.getSceneMin();
		glm::vec3 max = lander.getSceneMax();

		// set up bounding box for lander while we are at it
		//
		landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {
	cam.setPosition(glm::vec3(lander.getPosition()));
	cam.setTarget(lander.getPosition());
	cam.setDistance(50);


}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}



//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
} 

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent2(ofDragInfo dragInfo) {

	ofVec3f point;
	mouseIntersectPlane(ofVec3f(0, 0, 0), cam.getZAxis(), point);
	if (lander.loadModel(dragInfo.files[0])) {
		lander.setScaleNormalization(false);
//		lander.setScale(.1, .1, .1);
	//	lander.setPosition(point.x, point.y, point.z);
		lander.setPosition(1, 1, 0);

		bLanderLoaded = true;
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		cout << "Mesh Count: " << lander.getMeshCount() << endl;
	}
	else cout << "Error: Can't load model" << dragInfo.files[0] << endl;
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	ofVec2f mouse(mouseX, mouseY);
	ofVec3f rayPoint =	cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (lander.loadModel(dragInfo.files[0])) {
		bLanderLoaded = true;
		lander.setScaleNormalization(false);
		lander.setPosition(0, 0, 0);
		//cout << "number of meshes: " << lander.getNumMeshes() << endl;
		bboxList.clear();
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		//		lander.setRotation(1, 180, 1, 0, 0);

				// We want to drag and drop a 3D object in space so that the model appears 
				// under the mouse pointer where you drop it !
				//
				// Our strategy: intersect a plane parallel to the camera plane where the mouse drops the model
				// once we find the point of intersection, we can position the lander/lander
				// at that location.
				//

				// Setup our rays
				//
		glm::vec3 origin = cam.getPosition();
		glm::vec3 camAxis = cam.getZAxis();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
		if (hit) {
			// find the point of intersection on the plane using the distance 
			// We use the parameteric line or vector representation of a line to compute
			//
			// p' = p + s * dir;
			//
			glm::vec3 intersectPoint = origin + distance * mouseDir;

			// Now position the lander's origin at that intersection point
			//
			glm::vec3 min = lander.getSceneMin();
			glm::vec3 max = lander.getSceneMax();
			float offset = (max.y - min.y) / 2.0;
			lander.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

			// set up bounding box for lander while we are at it
			//
			landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		}
	}


}

//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = cam.getPosition();
	glm::vec3 camAxis = cam.getZAxis();
	glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}

// Author: Jonathan Nguyen
// Check if a collision is occuring
void ofApp::checkCollision() {
	
	//need to check this again for the smallest octrees
	

	ofVec3f min = lander.getSceneMin() + lander.getPosition();
	ofVec3f max = lander.getSceneMax() + lander.getPosition();
	Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

	nodeList.clear();

	if (octree.intersect(bounds, octree.root, nodeList)) {

		//get the point closest to the lander

		if (nodeList.size() > 0) {
			TreeNode closestNode = nodeList[0];
			glm::vec3 closest = octree.mesh.getVertex(nodeList[0].points[0]); //closest point from the closest node
			glm::vec3 ship = player.position; //position of the ship

			closestDistance = glm::distance(closest, ship);
			for (int i = 1; i < nodeList.size(); i++) { //check all the other 
				glm::vec3 vert = octree.mesh.getVertex(nodeList[i].points[0]);
				float nodeDist = glm::distance(vert, ship);

				if (nodeDist < closestDistance) { //if we find a closer point, set that one as cloest
					closestNode = nodeList[i];
					closestDistance = nodeDist;
				}

			}//end of the for loop


			//is the closest node to the terrain close enough to be a collision?
			if (closestDistance < epsilon) { // look up slides for formula
				norm = octree.mesh.getNormal(closestNode.points[0]);
				vel; //collision velocity

				if (sys.particles.size() > 0) {
					vel = sys.particles[0].velocity;
				}

				speed = glm::length(vel);
				collisionVel = fmaxf(collisionVel, speed);
				//cout << "Contact!" << endl;
				resolveCollision();

			}


		}

	}//end intersect
	
}

// Author: Jonathan Nguyen
// Resolve collision based on speed and area landed
void ofApp::resolveCollision() {

	if (bGame == true) {

		if (speed >= 5) {
			//too fast, crashed
			explodeEmitter.start();

			if (!bExplSound) {
				explSound.play(); //play the explosion sound only once
				bExplSound = true;

			}

			//move spacecraft somewhere, launch it I guess
			lander.setRotation(1, 180, 1, 0, 0);
			bLife = false; //Very much dead
			bGame = false;
		}

		if (speed > 0.5 && speed < 5 && bGame == true) {
			//not a smooth landing, bounce

			glm::vec3 impulseForce = 100 * (-glm::dot(vel, norm) * norm);
			//from the lecture slides

			if (sys.particles.size() > 0) {
				sys.particles[0].forces += impulseForce * ofGetFrameRate();
			}

		}
		else { //lander safely landed
			bGame = false; //stop the game... integrate should stop too
			
			//if we land inside the area
			if (targetArea.inside(Vector3(lander.getPosition().x, lander.getPosition().y,
				lander.getPosition().z))) {

				bInside = true;
				//cout << "Good landing" << endl;
			}
			else { //outside the target area
				bInside = false;
				//cout << "Bad landing" << endl;

			}

		}
	}
}

// Author: Jonathan Nguyen
// Setup cameras
void ofApp::setupCamera(){

	if (sys.particles.size() > 0) {
		player.position = sys.particles[0].position;
	}

	//Easy Cam setup
	cam.setPosition(150, 110, 200);
	cam.setTarget(lander.getPosition());
	cam.setDistance(30);

	//stationary tracking camera
	trackCam.setPosition(glm::vec3(0, 5, 0)); //stay fixed
	trackCam.lookAt(player.position);
	trackCam.setNearClip(0.1);

	float landX = player.position.x;
	float landY = player.position.y;
	float landZ = player.position.z;

	shipCam.setPosition(landX+1, landY+0.5, landZ+1);
	shipCam.lookAt(ofVec3f(), ofVec3f(-0.5, 0, -0.5));
	shipCam.setNearClip(0.1);
}

// Author: Jonathan Nguyen
// Update cameras to point target
void ofApp::updateCamera(){

	if (sys.particles.size() > 0) {
		player.position = sys.particles[0].position;
	}

	if(bFollow) {
		cam.setTarget(lander.getPosition());
		cam.setDistance(30);
	}

	//stationary, does not move
	trackCam.lookAt(player.position);

	//moves with lander
	float landX = player.position.x;
	float landY = player.position.y;
	float landZ = player.position.z;
	shipCam.setPosition(landX - 1, landY + 0.5, landZ - 1);
	shipCam.lookAt(ofVec3f(), ofVec3f(-0.5, 0, -0.5));
}

// Author: Jonathan Nguyen
//    For shader
void ofApp::loadVbo() {
	if (thrustEmitter.sys->particles.size() < 1) return;

	vector<ofVec3f> sizes;
	vector<ofVec3f> points;
	for (int i = 0; i < thrustEmitter.sys->particles.size(); i++) {
		points.push_back(thrustEmitter.sys->particles[i].position);
		sizes.push_back(ofVec3f(50));
	}
	// upload the data to the vbo
	//
	int total = (int)points.size();
	vbo.clear();
	vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
	vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);

}

// Author: Jonathan Nguyen
// Toggle between cameras in a linear fashion (unused)
void ofApp::rotateCamera()
{
	/* This is buggy. Do not use.
	//rotation is easyCam -> shipCam -> tracking cam
	
	currentCam++;
	if (currentCam == 3) {
		currentCam = 0;
	}
	
	theCam = &cameras[currentCam];
	
	*/
}

// Author: Jonathan Nguyen
//Get the current altitude using ray-intersect
void ofApp::calculateAltitude(){
	Vector3 landerPosition = Vector3(lander.getPosition().x,
		lander.getPosition().y, lander.getPosition().z);

	altitudeRadar = Ray(landerPosition, Vector3(0, -1, 0)); //straight down
	if (octree.intersect(altitudeRadar, octree.root, ground)) {

		//if intersected, get the altitude
		altitude = glm::distance(glm::vec3(lander.getPosition()),
			glm::vec3(octree.mesh.getVertex(ground.points[0])));


	}

}

// Author: Jonathan Nguyen
// Setup Lights (unused)
void ofApp::setupLights() {
	keyLight.setup();
	keyLight.enable();
	keyLight.setAreaLight(150, 150);
	keyLight.rotate(45, ofVec3f(0, 1, 0));
	keyLight.rotate(-45, ofVec3f(1, 0, 0));
	keyLight.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	keyLight.setAmbientColor(ofFloatColor(1, 1, 1));
	keyLight.setAmbientColor(ofFloatColor(1, 1, 1));
	keyLight.setPosition(300, 300, 300);

	fillLight.setup();
	fillLight.enable();
	fillLight.setSpotlight();
	fillLight.setScale(0.05);
	fillLight.setSpotlightCutOff(15);
	fillLight.setAttenuation(2, .001, .001);
	fillLight.rotate(-10, ofVec3f(1, 0, 0));
	fillLight.rotate(-45, ofVec3f(0, 1, 0));
	fillLight.setPosition(-5, 5, 5);

	rimLight.setup();
	rimLight.enable();
	rimLight.setSpotlight();
	fillLight.setScale(0.05);
	fillLight.setSpotlightCutOff(30);
	fillLight.setAttenuation(.2, .001, .001);
	fillLight.rotate(180, ofVec3f(0, 1, 0));
	rimLight.setPosition(0, 5, -7);

	shipLight.setup();
	shipLight.enable();
	shipLight.setPointLight();
	shipLight.setPosition(player.position);
	
}