/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeGUI helper class implementation
*/

#include <algorithm>
#include <stdexcept>

#include "guibase.h"

namespace GUI
{
	size_t Object::globalControlLastId = 0;

	Object::Object(const Fx::Rect & rc)
	{ 
		visible = true;
		enabled = true;
		alignHor = AlignManual;
		alignVer = AlignManual;
		borderSize = 0;
		windowRect = rc;
		desiredX = rc.x1;
		desiredY = rc.y1;
		desiredWidth = rc.x2 - rc.x1;
		desiredHeight = rc.y2 - rc.y1;
		color = Fx::MakeARGB(255, 255, 255, 255);
		clipChildren = false;
		contentsWidth = false;
		contentsHeight = false;
		layoutIsActive = false;
		cellWidth = 1;
		cellHeight = 1;
		cellX = 0;
		cellY = 0;
		layoutChanged = true;
		margin = 1.0;
		globalControlId = ++globalControlLastId;

		/// maybe this can happen one day
		if( globalControlLastId == ~(size_t)0 )
			std::runtime_error("Object::globalControlLastId overflow");
	}

	Object::~Object()
	{
		detach();
	}


	void Object::detach()
	{
		if(auto ptr = parent.lock())
		{
			ptr->removeChild(shared_from_this());
			parent.reset();
		}
	}

	bool Object::isRoot() const
	{
		return parent.expired();
	}


	void Object::setAlign(AlignType hor, AlignType ver)
	{
		if( alignHor != hor || alignVer != ver )
		{
			layoutChanged = false;
			alignHor = hor;
			alignVer = ver;
			updateLayout();
		}
	}


	void Object::insert(Pointer object)
	{
		Pointer thisptr = shared_from_this();

		assert(object != thisptr);
		children.push_back(object);

		object->parent = thisptr;
		object->layoutChanged = true;
		if(contentsWidth || contentsHeight)
			layoutChanged = true;
		updateLayout();
	}



	void Object::removeChild(const Object::Pointer & object)
	{
		Children::iterator it = std::find(children.begin(), children.end(), object);
		if(it != children.end())
			children.erase(it);
		if( contentsHeight || contentsWidth )
			layoutChanged = true;
	}

	void Object::updateLayout()
	{
		if (layoutIsActive == false || layoutChanged == false)
			return;

		if (Pointer parentPtr = parent.lock())
			parentPtr->calculateLayout(shared_from_this());

		for(Pointer object: children)
		{
			if( !object )
				continue;
			object->updateLayout();
		}
		layoutChanged = false;
	}


	void Object::callUpdate(float dt)
	{	
		if( layoutChanged )
			updateLayout();

		onUpdate(dt);

		for(Children::iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * object = it->get();
			if(object != NULL)
				object->callUpdate(dt);
		}
	}

	/*
	Fx::Rect Object::calculateChildrenRect() const
	{	
		// TODO:
		if( children.empty() )
			return Fx::Rect();
		Fx::Rect result;
		for(Children::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * object = it->get();
			if(object != NULL)
			{
				//resultobject->calculateChildrenRect();
			}
		}
		return result;
	}*/
	
	Fx::Rect Object::calculateContentsRect() const
	{
		Fx::Rect result;
		bool first = true;

		for(Children::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * child = it->get();
			Fx::Rect childRect = child->getRect();

			if( child->contentsWidth || child->contentsHeight )
			{
				Fx::Rect childContentsRect = child->calculateContentsRect();

				if( child->contentsWidth )
				{
					childRect.x1 = childContentsRect.x1;
					childRect.x2 = childContentsRect.x2;
				}
				if( child->contentsHeight )
				{
					childRect.y1 = childContentsRect.y1;
					childRect.y2 = childContentsRect.y2;
				}
			}
			if( first )
			{
				result = childRect;
				first = false;
			}
			else
			{
				result = Fx::Rect::Merge(result, childRect);
			}
		}
		return result;
	}

	void Object::enableLayouting( bool flag )
	{
		if( layoutIsActive != flag )
		{
			layoutIsActive = flag;
			//if( layouting )
			//	runLayout();
		}
	}


	void Object::callRender(Fx::RenderContext* context, const Fx::Rect & clipRect)
	{
		if(!visible)
			return;

		if(layoutChanged)
			updateLayout();

		Fx::Rect clip = Fx::Rect::Intersect(getRect(),clipRect);

		// if avialable area is zero - return
		if(clip.IsClean())
			return;

		// craw self
		const bool uiDebug = false;

		// draw children
		if( clipChildren )
			context->setClipping(clip);

		if(uiDebug)
			context->drawRect(getRect(), Fx::MakeARGB(255,64,255,64));
		onRender(context);
		// turn off clipping
		if( clipChildren )
			context->disableClipping();

		// render children
		for(Pointer object: children)
		{
			assert(object);
			object->callRender(context, clip);
		}		
	}

	void Object::findObject( const uiVec& vec, bool forceAll, std::function<ObjectIterator> fn)
	{
		if(!forceAll && (!visible || !enabled))
			return;
		Fx::Rect rect = getRect();

		if(!rect.TestPoint(vec[0], vec[1]))
			return;

		if(fn && fn(this))
			return;

		for(Children::iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * object = it->get();
			object->findObject(vec, forceAll, fn);
		}
	}

	bool Object::callMouseMove( int mouseId, const uiVec & vec, Object::MoveState state )
	{
		if(!visible || !enabled)
			return false;
		Fx::Rect rect = getRect();
		// test if point is outside
		if(!rect.TestPoint(vec[0], vec[1]))
			return false;
		// if we have any handler - use it, or update children instead
		if(onMouseMove(mouseId, vec, state))
			return true;
		
		// update children
		for(Children::iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * object = it->get();
			if( object->callMouseMove(mouseId, vec, state) )
				return true;
		}
		return false;
	}

	bool Object::callMouse( int mouseId, int key, int state, const uiVec & vec )
	{
		if(!visible || !enabled)
			return false;
		Fx::Rect rect = getRect();
		// test if point is outside
		if(!rect.TestPoint(vec[0], vec[1]))
			return false;

		// if we have any handler - use it, or update children instead
		if(onMouse(mouseId, key, state, vec))
			return true;
		// update children
		bool res = false;
		for(Children::iterator it = children.begin(); it != children.end(); ++it)
		{
			Object * object = it->get();
			if(object != NULL)
				res = res || object->callMouse(mouseId, key, state, vec);
			if( res )
				break;
		}
		return res;
	}

	void Object::callKeyClick(int key, int state)
	{
	}


	void Object::calculateLayout(const Object::Pointer & object)
	{
		Fx::Rect rect = getClientRect();
		Fx::Rect newRect = object->getRect();
		switch(object->alignHor)
		{
		case AlignManual:
			newRect.x1 = object->desiredX;
			newRect.x2 = object->desiredX + object->desiredWidth;
			break;
		case AlignCenter:		
			newRect.x1 = (rect.x1 + rect.x2 - object->desiredWidth)/2;
			newRect.x2 = (rect.x1 + rect.x2 + object->desiredWidth)/2;
			break;
		case AlignExpand:
			newRect.x1 = rect.x1;
			newRect.x2 = rect.x2;
			break;
		case AlignMin:		// left
			newRect.x1 = rect.x1;
			newRect.x2 = rect.x1 + object->desiredWidth;
			break;
		case AlignMax:		// right
			newRect.x1 = rect.x2 - object->desiredWidth;
			newRect.x2 = rect.x2;
			break;
		case AlignCell:		// TODO: implement
			break;
		}

		switch(object->alignVer)
		{
		case AlignManual:
			newRect.y1 = object->desiredY;
			newRect.y2 = object->desiredY + object->desiredHeight;
			break;
		case AlignCenter:
			newRect.y1 = (rect.y1 + rect.y2 - object->desiredHeight)/2;
			newRect.y2 = (rect.y1 + rect.y2 + object->desiredHeight)/2;
			break;
		case AlignExpand:
			newRect.y1 = rect.y1;
			newRect.y2 = rect.y2;
			break;
		case AlignMin:	// top
			newRect.y1 = rect.y1;
			newRect.y2 = rect.y1 + object->desiredHeight;
			break;			
		case AlignMax:	// bottom
			newRect.y1 = rect.y2 - object->desiredHeight;
			newRect.y2 = rect.y2;
			break;			
		case AlignCell:
			// TODO: implement
			break;
		}
		object->setRect(newRect);
	}

	bool Object::sendSignalUp(Object::Signal & msg)
	{
		if (!parent.expired())
		{
			auto ptr = parent.lock();
			if(!ptr->onSignalUp(msg))
				return ptr->sendSignalUp(msg);
			else
				return true;
		}
		return false;
	}

	bool Object::sendSignalDown(Object::Signal & msg)
	{
		/// 1. notify immediate children
		for( Children::iterator it = children.begin(); it != children.end(); ++it )
		{
			Object::Pointer object = *it;
			if( object && object->onSignalDown( msg ))	/// object can be occasionly dead
				return true;
		}
		/// 2. propagate signal further to children
		for( Children::iterator it = children.begin(); it != children.end(); ++it )
		{
			Object::Pointer object = *it;
			if (object && object->sendSignalDown(msg))
				return true;
		}
		return false;
	}


	void Object::setDesiredPos(float x, float y)
	{
		desiredX = x;
		desiredY = y;
	}


	void Object::setDesiredSize(float width, float height)
	{
		desiredWidth = width;
		desiredHeight = height;
	}


	void Object::setRect(const Fx::Rect & rect)
	{
		Fx::Rect oldRect = getRect();
		windowRect = rect;
		for(Children::iterator it = children.begin(); it != children.end(); ++it)
			calculateLayout(*it);
		onSize(rect.width(), rect.height());
	}

	Fx::Rect Object::getRect() const
	{
		return windowRect;
	}

	Fx::Rect Object::getClientRect() const
	{
		return Fx::Rect(windowRect.x1 + borderSize, windowRect.y1 + borderSize, windowRect.x2 - borderSize, windowRect.y2 - borderSize);
	}
} // namespace GUI

#if FUCK_THIS_2
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawRect(HGE * hge, const Fx::Rect& rect, Fx::FxRawColor color)
{
	hge->Gfx_RenderLine(rect.x1, rect.y1, rect.x2, rect.y1, color);
	hge->Gfx_RenderLine(rect.x2, rect.y1, rect.x2, rect.y2, color);
	hge->Gfx_RenderLine(rect.x2, rect.y2, rect.x1, rect.y2, color);
	hge->Gfx_RenderLine(rect.x1, rect.y2, rect.x1, rect.y1, color);
}

void drawRectSolid(HGE * hge, const Fx::Rect& rect, Fx::FxRawColor color)
{
	float z = 0;

	auto assign = [&](Fx::Vertex & target, float x, float y)
	{
		target.x = x;
		target.y = y;
		target.z = z;
		target.tx = 0;
		target.ty = 0;
		target.col = color;
	};
	
	Fx::Quad background;
	background.tex = 0;
	background.blend = 0;
	
	assign(background.v[0],rect.x1, rect.y1);
	assign(background.v[1],rect.x2, rect.y1);
	assign(background.v[2],rect.x2, rect.y2);
	assign(background.v[3],rect.x1, rect.y2);

	hge->Gfx_RenderQuad(&background);
}
#endif