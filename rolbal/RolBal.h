#pragma once

#include <blib/App.h>
#include <blib/RenderState.h>
#include <blib/gl/Vertex.h>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

namespace blib
{
	class Texture;
	class StaticModel;
}


class DebugDraw : public btIDebugDraw
{
	std::vector<blib::VertexP3C4> vertices;
	blib::RenderState renderstate;
	enum class Uniforms
	{
		ProjectionMatrix,
		CameraMatrix,
	};
	int debugMode;
public:
	DebugDraw(blib::ResourceManager* resourceManager);
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)	{	}

	void flush(blib::Renderer* renderer, const glm::mat4 &projectionMatrix, const glm::mat4 &cameraMatrix);

	virtual void reportErrorWarning(const char* warningString)		{ printf("%s\n", warningString); }
	virtual void draw3dText(const btVector3& location, const char* textString)		{	}
	virtual void setDebugMode(int debugMode)
	{
		this->debugMode = debugMode;
	}
	virtual int getDebugMode() const
	{
		return debugMode;
	}
};




class RolBal : public blib::App
{
public:
	btBroadphaseInterface*                  broadphase;
	btDefaultCollisionConfiguration*        collisionConfiguration;
	btCollisionDispatcher*                  dispatcher;
	btSequentialImpulseConstraintSolver*    solver;
	btDiscreteDynamicsWorld*                world;
	DebugDraw*								debugDraw;

	btCompoundShape ballShape;

	btRigidBody* ballBody;
	btRigidBody* floorBody;
	float ballDirection;
	glm::vec3 cameraPosition;

	std::vector<btRigidBody*> cubes;


	blib::RenderState renderState;
	enum class Uniforms
	{
		ProjectionMatrix,
		CameraMatrix,
		ModelMatrix,
		s_texture
	};
	blib::StaticModel* ballModel;
	blib::Texture* testTexture;

	blib::StaticModel* cubeModel;

	blib::KeyState lastKeyState;

	RolBal();
	virtual void init();
	virtual void update(double elapsedTime);
	virtual void draw();

};