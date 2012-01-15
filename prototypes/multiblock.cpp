#include "common.h"
#include "b2Tools.h"
#include "multiblock.h"

Multiblock::Multiblock()
	:body(NULL), cellWidth(15), selection(NULL), application(NULL)
{}

Multiblock::~Multiblock()
{
	if(body)
	{
		body->GetWorld()->DestroyBody(body);
	}
}

b2Body * Multiblock::getBody() const
{
	return body;
}

void Multiblock::init(const Pose2 & pose, b2World & world)
{
	if(body)
		return;
	// create world borders
	b2BodyDef bodyDef;
	bodyDef.position.Set(pose.position[0], pose.position[1]);
	bodyDef.angle = pose.orientation;
	bodyDef.type = b2_dynamicBody;
	bodyDef.angularDamping = 0.1;
	bodyDef.linearDamping = 0.1;
	bodyDef.fixedRotation = false;
	body = world.CreateBody(&bodyDef);
	body->SetUserData( this );
}

// Does body contains any shapes or exists
bool Multiblock::valid() const
{
	return body != NULL;
}
//
Multiblock::CellRef Multiblock::pick( const vec2f & worldPos ) const
{
	CellRef result;
	// 1. Obtain body pose 
	Pose2 pose = GetPose(body);
	// 2. Transform to local coordinates
	vec2f local = pose.projectPos(worldPos) / cellWidth;
	result.x = local[0] + 0.5;
	result.y = local[1] + 0.5;
	result.cell = (Cell*)getCell(result.x, result.y);
	return result;
}

void Multiblock::markSelected( Cell * cell )
{
	selection = cell;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// box2d specific
void Multiblock::onCellCreated( Multiblock::CellRef & ref, const CellDesc &desc)
{
	b2PolygonShape groundBox;
	groundBox.SetAsBox(cellWidth/2, cellWidth/2,  b2Vec2( ref.x * cellWidth, ref.y * cellWidth), 0);	
	b2FixtureDef def;
	def.shape = &groundBox;
	def.density = 1.0;
	def.isSensor = false;
	def.userData = ref.cell;
	ref.cell->data.fixture = body->CreateFixture(&def);	
	ref.cell->data.health = 100;
}

void Multiblock::onCellDestroyed(Multiblock::CellRef & ref, bool move)
{
	if(ref.cell->data.fixture)
	{
		body->DestroyFixture( ref.cell->data.fixture );
		ref.cell->data.fixture = NULL;
	}
}

void Multiblock::onLinkCreated(Link * link, int x, int y, AdjacentDirection dir, bool move)
{

}

const float impulseAdsorbedByCellMax = 100;	// ������������ �������, ����������� �������
const float impulseToCellDamage = 1.0;			// �������������� �������� � �����

const float impulseAdsorbedByLink = 20;


void Multiblock::hit( Multiblock::Cell * cell, const vec2f & pos, const vec2f & dir, float &impulseMod )
{	
	// 1. Adsorb impulse by cell
	// limit by impulseAdsorbedByCellMax, limit by current health
	float impulseAdsorbedByCell = std::min( std::min( cell->data.health / impulseToCellDamage, impulseAdsorbedByCellMax), impulseMod );	
	cell->data.health -= impulseToCellDamage * impulseAdsorbedByCell;
	impulseMod -= impulseAdsorbedByCell;
	// 2. Dissipate impulse to other cells
	Pose2 pose = ::GetPose(body);
	vec2f localDir = pose.projectVec( dir );
	for( size_t i = 0; i < 4; i ++ )
	{

	}
	/*
	P01, E01 projectile impulse / energy
	P01, E02 body impulse / energy
	D - body damage
	E01 + E02 = E11 + E12 + D

	1. All projectile energy is adsorbed
	E01 + E02 = E12 + D
	2. A
	Total projectile impulse = proj impulse P01 + body impulse P02
	{		
		E11
		E12
		body damage 
	}
	*/
	// 3. destroy cells
	if( cell->data.health <= 0 )
		destroyCell( cell->x, cell->y );

	// . Apply remaining impulse to body
	body->ApplyLinearImpulse( conv(dir * impulseMod), conv(pos) );
}