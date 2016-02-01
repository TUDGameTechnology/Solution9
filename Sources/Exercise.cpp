#include "pch.h"

#include <Kore/Application.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/Math/Random.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Audio/Mixer.h>
#include <Kore/Graphics/Image.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Log.h>
#include "ObjLoader.h"

#include "Collision.h"
#include "PhysicsWorld.h"
#include "PhysicsObject.h"

using namespace Kore;




namespace {
	const int width = 1024;
	const int height = 768;
	double startTime;
	Shader* vertexShader;
	Shader* fragmentShader;
	Program* program;

	float angle = 0.0f;


	bool left;
	bool right;
	bool up;
	bool down;

	// null terminated array of MeshObject pointers
	MeshObject* objects[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	// The sound to play for the winning condition
	Sound* winSound;


	// The view projection matrix aka the camera
	mat4 P;
	mat4 View;
	mat4 PV;

	vec3 cameraPosition;
	vec3 targetCameraPosition;
	vec3 oldCameraPosition;

	vec3 lookAt;
	vec3 targetLookAt;
	vec3 oldLookAt;

	MeshObject* sphere;
	PhysicsObject* po;

	PhysicsWorld physics;
	

	// uniform locations - add more as you see fit
	TextureUnit tex;
	ConstantLocation pvLocation;
	ConstantLocation mLocation;

	/************************************************************************/
	/* Solution 1.2 - Initialize the box collider                           */
	/************************************************************************/
	BoxCollider boxCollider(vec3(-46.0f, -4.0f, 44.0f), vec3(10.6f, 4.4f, 4.0f));
	bool playedSound = false;

	double lastTime;

	void update() {
		double t = System::time() - startTime;
		double deltaT = t - lastTime;

		lastTime = t;
		Kore::Audio::update();
		
		Graphics::begin();
		Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, 0xff9999FF, 1000.0f);

		program->set();

		// set the camera

		angle += 0.3f * deltaT;

		float x = 0 + 10 * Kore::cos(angle);
		float z = 0 + 10 * Kore::sin(angle);
		
		targetCameraPosition.set(x, 2, z);

		
		targetCameraPosition = physics.physicsObjects[0]->GetPosition();
		targetCameraPosition = targetCameraPosition + vec3(-10, 5, 10);
		vec3 targetLookAt = physics.physicsObjects[0]->GetPosition();

		
		// Interpolate the camera to not follow small physics movements
		float alpha = 0.3f;

		cameraPosition = oldCameraPosition * (1.0f - alpha) + targetCameraPosition * alpha;
		oldCameraPosition = cameraPosition;

		lookAt = oldLookAt * (1.0f - alpha) + targetLookAt * alpha;
		oldLookAt = lookAt;
		
		

		// Follow the ball with the camera
		P = mat4::Perspective(60, (float)width / (float)height, 0.1f, 100);
		View = mat4::lookAt(cameraPosition, lookAt, vec3(0, 1, 0)); 
		PV = P * View;


		Graphics::setMatrix(pvLocation, PV);





		// iterate the MeshObjects
		MeshObject** current = &objects[0];
		while (*current != nullptr) {
			// set the model matrix
			Graphics::setMatrix(mLocation, (*current)->M);

			(*current)->render(tex);
			++current;
		} 

		

		physics.Update(deltaT);


		PhysicsObject** currentP = &physics.physicsObjects[0];
	

		// Handle mouse inputs
		float forceX = 0.0f;
		float forceZ = 0.0f;
		if (up) forceX += 1.0f;
		if (down) forceX -= 1.0f;
		if (left) forceZ -= 1.0f;
		if (right) forceZ += 1.0f;

		// Apply gravity
		vec3 force(forceX, 0.0f, forceZ);
		force = force * 20.0f;
		(*currentP)->ApplyForceToCenter(force);



		while (*currentP != nullptr) {
			(*currentP)->UpdateMatrix();
			Graphics::setMatrix(mLocation, (*currentP)->Mesh->M);
			(*currentP)->Mesh->render(tex);
			++currentP;
		}


		/************************************************************************/
		/* Solution 1.2 - Check the box collider for collision                  */
		/************************************************************************/
		PhysicsObject* SpherePO = physics.physicsObjects[0];
		bool result = SpherePO->Collider.IntersectsWith(boxCollider);
		if (result && !playedSound) {
			playedSound = true;
			Mixer::play(winSound);
		}
			

		Graphics::end();
		Graphics::swapBuffers();
	}

	void SpawnSphere(vec3 Position, vec3 Velocity) {
		PhysicsObject* po = new PhysicsObject();
		po->SetPosition(Position);
		po->Velocity = Velocity;
		
		po->Collider.radius = 0.5f;

		po->Mass = 5;
		po->Mesh = sphere;
			
		// The impulse should carry the object forward

		po->ApplyImpulse(Velocity);
		physics.AddObject(po);
	}

	void keyDown(KeyCode code, wchar_t character) {
		if (code == Key_Space) {
		} else if (code == Key_Up) {
			up = true;
		} else if (code == Key_Down) {
			down = true;
		} else if (code == Key_Left) {
			right = true;
		} else if (code == Key_Right) {
			left = true;
		}
	}

	void keyUp(KeyCode code, wchar_t character) {
		if (code == Key_Up) {
			up = false;
		} else if (code == Key_Down) {
			down = false;
		} else if (code == Key_Left) {
			right = false;
		} else if (code == Key_Right) {
			left = false;
		}
	}

	void mouseMove(int x, int y, int movementX, int movementY) {

	}
	
	void mousePress(int button, int x, int y) {

	}

	

	void mouseRelease(int button, int x, int y) {
		
	}

	void init() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);

		// This defines the structure of your Vertex Buffer
		VertexStructure structure;
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);

		program = new Program;
		program->setVertexShader(vertexShader);
		program->setFragmentShader(fragmentShader);
		program->link(structure);

		tex = program->getTextureUnit("tex");
		pvLocation = program->getConstantLocation("PV");
		mLocation = program->getConstantLocation("M");

		objects[0] = new MeshObject("Test.obj", "Level/basicTiles6x6.png", structure);
		objects[1] = new MeshObject("Level/Level_yellow.obj", "Level/basicTiles3x3yellow.png", structure);
		objects[2] = new MeshObject("Level/Level_red.obj", "Level/basicTiles3x3red.png", structure);

		sphere = new MeshObject("ball_at_origin.obj", "Level/unshaded.png", structure);

		float pos = -10.0f;
		SpawnSphere(vec3(-pos, 5.5f, pos), vec3(0, 0, 0));

		physics.meshCollider.mesh = objects[0];

		// Sound source: http://opengameart.org/content/level-up-sound-effects
		
		/************************************************************************/
		/* Task 1.2: Play this sound when the goal is reached                   */
		/************************************************************************/
		winSound = new Sound("chipquest.wav");
		Mixer::play(winSound);

		Graphics::setRenderState(DepthTest, true);
		Graphics::setRenderState(DepthTestCompare, ZCompareLess);

		Graphics::setTextureAddressing(tex, U, Repeat);
		Graphics::setTextureAddressing(tex, V, Repeat);

		

		

	}
}

int kore(int argc, char** argv) {
	Application* app = new Application(argc, argv, width, height, 0, false, "Exercise8");
	Kore::Mixer::init();
	Kore::Audio::init();

	init();

	app->setCallback(update);

	startTime = System::time();
	lastTime = 0.0f;
	
	
	
	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;

	app->start();

	delete app;
	
	return 0;
}
