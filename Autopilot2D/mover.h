#ifndef MOVER_INCLUDED
#define MOVER_INCLUDED

#include "objectManager.h"
#include "gameObject.h"
#include "device.h"
//////////////////////////////////////////////////////////////////////////
// ��� �� ������ �����: ��������� � �����, ��������� � ����� � ������������
//////////////////////////////////////////////////////////////////////////

class Mover: public Device//, public WorldObject
{
	friend class ObjectManager;	
public:
	class Definition: public DeviceDef
	{
	public:
		Definition(ObjectManager & manager);
		virtual ~Definition();
		/// pathfinder settings
		int adjacency;
		bool bounds;
		bool smooth;	
		bool heuristic;
		pathProject::PathProcess * pathProcess;	// for grid pathfinding
	};
	typedef Definition MoverDef;
	class Driver;
	friend class Driver;
	enum State
	{
		Idle,
		//Wait,
		Moving
	};

	struct Listener
	{
		virtual void onSetState(int state,float value)=0;
	}*listener;

	Mover(MoverDef *def);
	//int readDesc(IOBuffer &context);
	//int writeDesc(IOBuffer &context);	
	int readState(IO::StreamIn &buffer);
	int writeState(IO::StreamOut &buffer);
	
	virtual ~Mover();
	virtual void stop();
	virtual void update(float dt)=0;

	virtual void updateTask();				/// update the path
	virtual State getState() const;
	virtual Driver * getDriver();
	virtual const Driver * getDriver() const;
	virtual void setDriver(Driver * d);
	virtual void clearDriver();	

	virtual void directionControl(const Pose::vec& direction, float speed) = 0;
	virtual float getMaxVelocity(int dir) const { return 0.f;}
#ifdef DEVICE_RENDER
	virtual void render(HGE * hge);
#endif
	pathProject::PathProcess * pathProcess();
	Pose::vec getLastControl() const;
protected:	
	Pose::vec lastControl;
	Driver * driver;
	State state;	
};

typedef Mover::Definition MoverDef;

class Mover::Driver : public PathFinder::Client
{
public:
	enum PathEvent
	{
		taskNew,
		taskSuccess,
		taskFail,
	};

	Driver(Mover* mover);
	virtual ~Driver(){};

	virtual void update(float dt);
	virtual void updatePath(float dt);		/// update current waypoint
	virtual void updateTask();				/// update current path
#ifdef DEVICE_RENDER
	virtual void render(HGE * hge);
#endif
	virtual int obstaclesAdd(const Geom::Traectory2 &tr,float size);	// adds object to evade
	virtual void obstaclesClear();

//	virtual int getNearest(Pose::pos * targets,int count)=0;	// find nearest target, returns index
	virtual void walk(const Pose & pose);
	virtual void walk(const Pose::pos & target);				// set moving target
	virtual void face(const Pose::vec &t);

	virtual Mover * getMover() const {return mover;}

	class Listener
	{
	public:
		virtual void moverEvent(Mover *mover,int state)=0;
	}*listener;

	float rayCast(const Pose::vec& offset,const Pose::vec& dir)const;

	int waypointsCount() const;
	int waypoints(vec2f * points, int max = -1) const;				// copy local waypoints to "points" array
	//virtual int canReach(const vec2f &target, float maxDistance=0.0f); // 
	virtual float pathLength();		// current path length
	virtual float timeToImpact();	// time to impact in described obstacle
	Pose::vec pathDirection();
	Pose::vec pathError();
	
protected:
	typedef PathFinder::Path Path;
	Path path;
	bool waiting;
	virtual void getPath(Path & ptr, bool success);
	/*
	struct Waypoint2
	{
		vec2f v;
		int action;
	};
	std::vector<Waypoint2> path;*/
	typedef std::pair<Geom::Traectory2,float> Obstacle;
	typedef std::vector< Obstacle > Obstacles;	
	Obstacles obstacles;

	Pose::pos targetDir;
	Pose::pos target;
	struct MoveTask
	{
		bool target;
		bool dir;
		MoveTask();
		void clear();
	}task;

	uint32 pathCurrent;		
	float minDistance;
	Mover * mover;
protected:
	//int buildPath();
	bool tryNextWaypoint();					// try to get next waypoint
	const Path::Waypoint2 * current();
	void clearTask();
	pathProject::PathProcess * pathProcess();
};

Mover::Driver * createFlockingDriver(Mover* m);
Mover::Driver * createVODriver(Mover* m);
Mover::Driver * createVO2Driver(Mover* m);

void drawArrow(HGE * hge,const vec2f &from,const vec2f &to,DWORD color,float width=0.01,float length=0.02);
void drawPoint(HGE * hge,const vec2f &pos,DWORD color,float size,int style=0);
void drawLine(HGE * hge,const vec2f &from,const vec2f &to,DWORD color);
#endif
