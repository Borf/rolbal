#include "RolBal.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <blib/Renderer.h>
#include <blib/LineBatch.h>
#include <blib/ResourceManager.h>
#include <blib/Window.h>
#include <blib/StaticModel.h>
#include <blib/Math.h>


RolBal::RolBal()
{
	appSetup.renderer = blib::AppSetup::GlRenderer;
	appSetup.window.setWidth(1920);// resolution.x - 100;
	appSetup.window.setHeight(1079);// resolution.y - 100;
	appSetup.border = false;
	appSetup.icon = 0;
	appSetup.vsync = false;
	appSetup.title = "Rolbal";
	appSetup.joystickDriver = blib::AppSetup::JoystickPreference::DirectInput;
	appSetup.vsync = false;

	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

}


void RolBal::init()
{
	renderState.depthTest = true;
	renderState.activeShader = resourceManager->getResource<blib::Shader>("simple");
	renderState.activeShader->bindAttributeLocation("a_position", 0);
	renderState.activeShader->bindAttributeLocation("a_texcoord", 1);
	renderState.activeShader->bindAttributeLocation("a_normal", 2);
	renderState.activeShader->setUniformName(Uniforms::ProjectionMatrix, "projectionMatrix", blib::Shader::Mat4);
	renderState.activeShader->setUniformName(Uniforms::ModelMatrix, "modelMatrix", blib::Shader::Mat4);
	renderState.activeShader->setUniformName(Uniforms::CameraMatrix, "cameraMatrix", blib::Shader::Mat4);
	renderState.activeShader->setUniformName(Uniforms::s_texture, "s_texture", blib::Shader::Int);
	renderState.activeShader->finishUniformSetup();
	renderState.activeShader->setUniform(Uniforms::s_texture, 0);
	renderState.cullFaces = blib::RenderState::CullFaces::CCW;


	testTexture = resourceManager->getResource<blib::Texture>("assets/textures/test.png");
	testTexture->setTextureRepeat(true);
	ballModel = new blib::StaticModel("assets/models/coolsphere.dae.json", resourceManager, renderer);
	ballModel->meshes[0]->material.texture = resourceManager->getResource<blib::Texture>("assets/models/coolsphere.png");

	cubeModel = new blib::StaticModel("assets/models/cube.dae.json", resourceManager, renderer);
	cubeModel->meshes[0]->material.texture = resourceManager->getResource<blib::Texture>("assets/models/cube.png");

	world->setGravity(btVector3(0, -9.8f, 0));
	debugDraw = new DebugDraw(resourceManager);
	debugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	world->setDebugDrawer(debugDraw);



	{
		btCollisionShape* groundShape = new btBoxShape(btVector3(1000, 1, 1000));

		btVector3 inertia;
		groundShape->calculateLocalInertia(0, inertia);
		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 0, 0));
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo cInfo(0, myMotionState, groundShape, inertia);
		floorBody = new btRigidBody(cInfo);
		world->addRigidBody(floorBody);
		floorBody->setRestitution(0);
		floorBody->setFriction(1);
	}

	{
		float mass = 1;

		{
			btCollisionShape* initialBallShape = new btSphereShape(1);
			btTransform localTransform;
			localTransform.setIdentity();
			localTransform.setOrigin(btVector3(0, 0, 0));
			ballShape.addChildShape(localTransform, initialBallShape);
		}


		btTransform ballTransform;
		ballTransform.setIdentity();
		ballTransform.setOrigin(btVector3(0, 2, 0));
		btDefaultMotionState* myMotionState = new btDefaultMotionState(ballTransform);
		btVector3 inertia;
		ballShape.calculateLocalInertia(mass, inertia);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, &ballShape, inertia);
		ballBody = new btRigidBody(cInfo);
		ballBody->forceActivationState(DISABLE_DEACTIVATION);
		world->addRigidBody(ballBody);

		ballBody->setRestitution(0);
		ballBody->setFriction(1);
		ballBody->setDamping(0.9f, 0.5);
	}


	for (int x = 0; x < 5; x++)
		for (int y = 0; y < 5; y++)
	{
		float mass = 0.5f;
		btCollisionShape* blockShape = new btBoxShape(btVector3(0.25f, 0.25f, 0.25f));
		btVector3 inertia;
		blockShape->calculateLocalInertia(mass, inertia);
		btTransform blockTransform;
		blockTransform.setIdentity();
		blockTransform.setOrigin(btVector3(0+x, 1.25f, 4+y));
		btDefaultMotionState* myMotionState = new btDefaultMotionState(blockTransform);
		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, blockShape, inertia);
		btRigidBody* blockBody = new btRigidBody(cInfo);
		blockBody->forceActivationState(DISABLE_DEACTIVATION);
		world->addRigidBody(blockBody);

		blockBody->setRestitution(0);
		blockBody->setFriction(1);
		blockBody->setDamping(0.9f, 0.5);
		cubes.push_back(blockBody);
	}


	ballDirection = blib::math::pif / 2;
}


void RolBal::update(double elapsedTime)
{
	if (keyState.isPressed(blib::Key::ESC))
		running = false;

	world->stepSimulation(elapsedTime, 10);

	glm::vec3 ballPosition(ballBody->getCenterOfMassPosition().x(), ballBody->getCenterOfMassPosition().y(), ballBody->getCenterOfMassPosition().z());
	cameraPosition = ballPosition + 3.0f * glm::vec3(cos(ballDirection + blib::math::pif), 1, sin(ballDirection + blib::math::pif));

	if (keyState.isPressed(blib::Key::UP))
		ballBody->applyCentralImpulse(0.1f * btVector3(cos(ballDirection), 0, sin(ballDirection)));
	if (keyState.isPressed(blib::Key::DOWN))
		ballBody->applyCentralImpulse(0.1f * btVector3(cos(ballDirection + blib::math::pif), 0, sin(ballDirection + blib::math::pif)));
	if (keyState.isPressed(blib::Key::LEFT))
		ballDirection -= elapsedTime * 1.5f;
	if (keyState.isPressed(blib::Key::RIGHT))
		ballDirection += elapsedTime * 1.5f;


	if (keyState.isPressed(blib::Key::SPACE) && !lastKeyState.isPressed(blib::Key::SPACE))
	{
		btCollisionShape* initialBallShape = new btBoxShape(btVector3(0.1f, 0.1f, 0.1f));
		btTransform localTransform;
		localTransform.setIdentity();

		float theta = blib::math::randomFloat(0, 2 * blib::math::pif);
		float phi = blib::math::randomFloat(0, 2 * blib::math::pif);
		
		float x1 = glm::cos(theta) * glm::cos(phi);
		float y1 = glm::cos(theta) * glm::sin(phi);
		float z1 = glm::sin(theta);

		localTransform.setOrigin(btVector3(x1, y1, z1));
		localTransform.setRotation(btQuaternion(blib::math::randomFloat(), blib::math::randomFloat(), blib::math::randomFloat(), blib::math::randomFloat()));
		ballShape.addChildShape(localTransform, initialBallShape);
	}

	class ContactSensorCallback : public btCollisionWorld::ContactResultCallback 
	{
	public:
		RolBal* rolbal;

		struct Res
		{
			btTransform transform;
			btRigidBody* otherObject;

		};

		std::vector<Res> results;

		ContactSensorCallback(RolBal* rolbal)
		{
			this->rolbal = rolbal;
		}
		virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
		{
			btVector3 pt; // will be set to point of collision relative to body
			if (colObj0Wrap->m_collisionObject == rolbal->floorBody || colObj1Wrap->m_collisionObject == rolbal->floorBody)
				return 0;

			if (cp.getDistance() > 0.001)
				return 0;

			if (colObj0Wrap->m_collisionObject == rolbal->ballBody)
			{
				pt = cp.m_localPointA;
				printf("Collision 1!\n");
				btRigidBody* otherObject = (btRigidBody*)colObj1Wrap->m_collisionObject;
				btVector3 otherPosition = otherObject->getCenterOfMassPosition();


				btScalar m[16];
				rolbal->ballBody->getWorldTransform().getOpenGLMatrix(m);
				glm::mat4 ballMatrix = glm::make_mat4(m);
				otherObject->getWorldTransform().getOpenGLMatrix(m);
				glm::mat4 objMatrix = glm::make_mat4(m);


				glm::vec3 diff = glm::vec3(glm::inverse(ballMatrix) * glm::vec4(otherPosition.x(), otherPosition.y(), otherPosition.z(), 1));

				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(diff.x, diff.y, diff.z));



//				rolbal->world->removeRigidBody(otherObject);
//				rolbal->ballShape.addChildShape(transform, otherObject->getCollisionShape());


				results.push_back(Res{ transform, otherObject});
			}
			else if (colObj1Wrap->m_collisionObject == rolbal->ballBody)
			{
				pt = cp.m_localPointB;
				printf("Collision 2!\n");
			}
			return 0;
		}
	} callback(this);
	world->contactTest(ballBody, callback);

	for (const ContactSensorCallback::Res & r: callback.results)
	{
		world->removeRigidBody(r.otherObject);
		ballShape.addChildShape(r.transform, r.otherObject->getCollisionShape());
	}




	lastKeyState = keyState;
}

void RolBal::draw()
{
	glm::vec3 ballPosition(ballBody->getCenterOfMassPosition().x(), ballBody->getCenterOfMassPosition().y(), ballBody->getCenterOfMassPosition().z());


	renderer->clear(glm::vec4(1.0f, 0.6f, 0.8f, 1.0f), blib::Renderer::Color | blib::Renderer::Depth);
	glm::mat4 projectionMatrix = glm::perspective(90.0f, (float)window->getWidth() / window->getHeight(), 0.1f, 500.0f);
	glm::mat4 cameraMatrix = glm::lookAt(cameraPosition, ballPosition, glm::vec3(0, 1, 0));
	renderState.depthTest = true;
	renderState.activeShader->setUniform(Uniforms::CameraMatrix, cameraMatrix);
	renderState.activeShader->setUniform(Uniforms::ProjectionMatrix, projectionMatrix);



	float h = 1;
	std::vector<blib::VertexP3T2N3> verts;
	verts.push_back(blib::VertexP3T2N3(glm::vec3(-1000, h, -1000), glm::vec2(0, 0), glm::vec3(0, 1, 0)));
	verts.push_back(blib::VertexP3T2N3(glm::vec3(1000, h, 1000), glm::vec2(100, 100), glm::vec3(0, 1, 0)));
	verts.push_back(blib::VertexP3T2N3(glm::vec3(1000, h, -1000), glm::vec2(100, 0), glm::vec3(0, 1, 0)));

	verts.push_back(blib::VertexP3T2N3(glm::vec3(-1000, h, 1000), glm::vec2(0, 100), glm::vec3(0, 1, 0)));
	verts.push_back(blib::VertexP3T2N3(glm::vec3(1000, h, 1000), glm::vec2(100, 100), glm::vec3(0, 1, 0)));
	verts.push_back(blib::VertexP3T2N3(glm::vec3(-1000, h, -1000), glm::vec2(0, 0), glm::vec3(0, 1, 0)));

	renderState.activeTexture[0] = testTexture;
	renderState.activeShader->setUniform(Uniforms::ModelMatrix, glm::mat4());
	renderer->drawTriangles(verts, renderState);
	 


	btScalar m[16];
	ballBody->getWorldTransform().getOpenGLMatrix(m);
	glm::mat4 ballMatrix = glm::make_mat4(m);


	renderState.activeShader->setUniform(Uniforms::ModelMatrix, ballMatrix);
	ballModel->draw(renderState, renderer, -1);

	for (int i = 1; i < ballShape.getNumChildShapes(); i++)
	{
		ballShape.getChildTransform(i).getOpenGLMatrix(m);
		renderState.activeShader->setUniform(Uniforms::ModelMatrix, glm::scale(ballMatrix * glm::make_mat4(m), glm::vec3(.25f, .25f, .25f)));
		cubeModel->draw(renderState, renderer, -1);
	}

	for (auto body : cubes)
	{
		if (!body->isInWorld())
			continue;
		body->getWorldTransform().getOpenGLMatrix(m);
		renderState.activeShader->setUniform(Uniforms::ModelMatrix, glm::scale(glm::make_mat4(m), glm::vec3(.25f, .25f, .25f)));
		cubeModel->draw(renderState, renderer, -1);
	}



#if _DEBUG
	world->debugDrawWorld();
	debugDraw->flush(renderer, projectionMatrix, cameraMatrix);
#endif

}




















DebugDraw::DebugDraw(blib::ResourceManager* resourceManager)
{
	renderstate.activeShader = resourceManager->getResource<blib::Shader>("bulletdebug");
	renderstate.activeShader->setUniformName(Uniforms::ProjectionMatrix, "projectionMatrix", blib::Shader::Mat4);
	renderstate.activeShader->setUniformName(Uniforms::CameraMatrix, "cameraMatrix", blib::Shader::Mat4);
	renderstate.activeShader->finishUniformSetup();
	renderstate.depthTest = false;
}


void DebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	if (vertices.size() > 1000000)
		return;
	glm::vec4 c(color.m_floats[0], color.m_floats[1], color.m_floats[2], 1.0f);
	vertices.push_back(blib::VertexP3C4(glm::vec3(from.m_floats[0], from.m_floats[1], from.m_floats[2]), c));
	vertices.push_back(blib::VertexP3C4(glm::vec3(to.m_floats[0], to.m_floats[1], to.m_floats[2]), c));
}

void DebugDraw::flush(blib::Renderer* renderer, const glm::mat4 &projectionMatrix, const glm::mat4 &cameraMatrix)
{
	if (!vertices.empty())
	{
		renderstate.activeShader->setUniform(Uniforms::ProjectionMatrix, projectionMatrix);
		renderstate.activeShader->setUniform(Uniforms::CameraMatrix, cameraMatrix);
		renderer->drawLines(vertices, renderstate);
		vertices.clear();
	}
}
