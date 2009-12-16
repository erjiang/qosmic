/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009 by David Bitseff                       *
 *   dbitsef@zipcon.net                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <math.h>

#include "triangle.h"
#include "logger.h"
#include "nodeitem.h"
#include "xfedit.h"

uint Triangle::zpos = 0;

Triangle::Triangle( FigureEditor* c, flam3_xform* x, BasisTriangle* b, int idx)
: canvas(c), m_xform(x), basisTriangle(b), m_index(idx), cList(), nList(),
  m_edgeLine(0.,0.,0.,0.), m_edgeType(NoEdge)
{
	logFinest("Triangle::Triangle : enter");
	setTransform(QTransform(basisTriangle->coordTransform()));
	NodeItem *el;
	el = new NodeItem( canvas, this, NODE_O, "O" );
	addNode(el);
	el = new NodeItem( canvas, this, NODE_X, "X" );
	addNode(el);
	el = new NodeItem( canvas, this, NODE_Y, "Y" );
	addNode(el);
	TriangleCoords tc = basisTriangle->getCoords(m_xform->c);
	setPoints(tc);
	moveToFront();
	setBrush(Qt::NoBrush);
	setPen(QPen(Qt::gray));
}

Triangle::~Triangle()
{
    NodeItem *node;
    TriangleNodesIterator it( nList );
    while ( it.hasNext() )
	{
        node = it.next();
		canvas->removeItem(node);
		delete node;
    }
    hide();
}

void Triangle::resetPosition()
{
	m_xform->c[0][0] = 1.0;
	m_xform->c[0][1] = 0.0;
	m_xform->c[1][0] = 0.0;
	m_xform->c[1][1] = 1.0;
	m_xform->c[2][0] = 0.0;
	m_xform->c[2][1] = 0.0;

	TriangleCoords tc = basisTriangle->getCoords(m_xform->c);
	setPoints(tc);
}

void Triangle::addNode( NodeItem *node )
{
    node->setTriangle(this);
	node->setParentItem(this);
	canvas->addItem(node);
    nList.append( node );
}

TriangleNodes& Triangle::getNodes()
{
    return nList;
}

TriangleCoords Triangle::getCoords()
{
    return cList;
}

int Triangle::type() const
{
    return RTTI;
}

flam3_xform* Triangle::xform()
{
    return m_xform;
}

void Triangle::basisScaledSlot()
{
	setTransform(QTransform(basisTriangle->coordTransform()));
	TriangleCoords tc = basisTriangle->getCoords(m_xform->c);
	setPoints(tc);

	// only update the brush matrix for selected triangles
	if (this == canvas->getSelectedTriangle())
	{
		QBrush b(brush());
		b.setMatrix(basisTriangle->coordTransform());
		setBrush(b);
	}
}

void Triangle::setPoints(TriangleCoords& points)
{
	logFinest("Triangle::setPoints : enter");
	cList = points;
    if (points.size() != 3)
        return;
    int n = 0;
    NodeItem *node;
    TriangleNodesIterator it( nList );

    while ( it.hasNext() )
	{
		node = it.next();
		QPointF npos = mapToItem(node, cList[n]);
        node->movePoint(npos.x(), npos.y());
        n++;
    }
    QGraphicsPolygonItem::setPolygon(cList);
}

void Triangle::moveToFront()
{
    zpos++;
    setZValue(zpos);
    zpos++;
    TriangleNodesIterator it( nList );
    while ( it.hasNext() )
		it.next()->setZValue( zpos );
}

int Triangle::nextZPos()
{
	return ++zpos;
}

void Triangle::show()
{
    QGraphicsPolygonItem::show();
    TriangleNodesIterator it( nList );
    while ( it.hasNext() )
		it.next()->show();

}

void Triangle::setVisible(bool flag)
{
	QGraphicsPolygonItem::setVisible(flag);
	TriangleNodesIterator it( nList );
	while ( it.hasNext() )
		it.next()->setVisible(flag);
}

int Triangle::index() const
{
	return m_index;
}

QRectF Triangle::boundingRect()
{
	return QGraphicsPolygonItem::boundingRect().adjusted(-.4,-.4,.8,.8);
}

/**
 *  This one is called by the currently moving NodeItem
 */
void Triangle::moveEdges()
{
    TriangleNodesIterator it( nList );
    NodeItem *node;
    cList.clear();
	while ( it.hasNext() )
	{
		node = it.next();
		cList <<  mapFromScene(node->pos());
	}
    QGraphicsPolygonItem::setPolygon( cList );
	adjustSceneRect();
	coordsToXForm();
}

void Triangle::scale(double dx, double dy, QPointF cpos)
{
	double tx =  cpos.x();
	double ty =  cpos.y();
	QTransform trans = transform();
	// scale
	translate(tx, ty);
	QGraphicsPolygonItem::scale(dx, dy);
	translate(-tx, -ty);

	QPolygonF pa = polygon();
	TriangleNodesIterator it( nList );
    NodeItem *node;
	int n = 0;
	// rebuild triangle + nodes
    while ( it.hasNext() )
	{
		node = it.next();
		QPointF p = mapToScene(pa[n]);
		node->setPos( p );
		n++;
    }
	setTransform(trans);
	it = TriangleNodesIterator( nList );
	cList.clear();
	while ( it.hasNext() )
	{
		node = it.next();
		cList << mapFromScene(node->pos());
	}
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}

void Triangle::rotate(double rad, QPointF cpos)
{
	double tx =  cpos.x();
	double ty =  cpos.y();
	QTransform trans = transform();
	// rotate
	translate(tx, ty);
	QGraphicsPolygonItem::rotate(rad);
	translate(-tx, -ty);

	QPolygonF pa = polygon();
	TriangleNodesIterator it( nList );
    NodeItem *node;
	int n = 0;
	// rebuild triangle + nodes
    while ( it.hasNext() )
	{
		node = it.next();
		QPointF p = mapToScene(pa[n]);
		node->setPos( p );
		n++;
    }
	setTransform(trans);
	it = TriangleNodesIterator( nList );
	cList.clear();
	while ( it.hasNext() )
	{
		node = it.next();
		cList << mapFromScene(node->pos());
	}
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}


void Triangle::flipHorizontally()
{
	flipHorizontally(polygon().boundingRect().center());
}

void Triangle::flipVertically()
{
	flipVertically(polygon().boundingRect().center());
}

void Triangle::flipHorizontally(QPointF center)
{
	double tx = center.x();
	QPolygonF pa = polygon();
	int n = 0;
	foreach (QPointF p, pa)
	{
		p.rx()  =  p.x() - 2.*(p.x() - tx) ;
		pa[n++] = p;
	}
	TriangleNodesIterator it( nList );
    NodeItem *node;
	n = 0;
	cList.clear();
    while ( it.hasNext() )
	{
		node = it.next();
		QPointF p = mapToScene(pa[n++]);
		node->setPos( p );
        cList << mapFromScene(node->pos());
    }
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}

void Triangle::flipVertically(QPointF center)
{
	double ty = center.y();
	QPolygonF pa = polygon();
	int n = 0;
	foreach (QPointF p, pa)
	{
		p.ry()  =  p.y() - 2.*(p.y() - ty) ;
		pa[n++] = p;
	}
	TriangleNodesIterator it( nList );
    NodeItem *node;
	n = 0;
	cList.clear();
    while ( it.hasNext() )
	{
		node = it.next();
		QPointF p = mapToScene(pa[n++]);
		node->setPos( p );
        cList << mapFromScene(node->pos());
    }
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}

/**
 *  This one is called by the FigureEditor
 */
void Triangle::moveBy(double dx, double dy)
{
	TriangleNodesIterator it( nList );
    NodeItem *node;
 	cList.clear();
    while ( it.hasNext() )
	{
		node = it.next();
		node->movePoint( dx, dy );
        cList << mapFromScene(node->pos());
    }
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}

BasisTriangle* Triangle::basis() const
{
	return basisTriangle;
}

const QMatrix& Triangle::getCoordinateTransform()
{
    return basisTriangle->coordTransform();
}

void Triangle::coordsToXForm()
{
	logFinest("Triangle::coordsToXForm : enter");
	basisTriangle->applyTransform(cList, m_xform->c);
}

void Triangle::setNodeColor(const QColor& c, const QColor& s)
{
	TriangleNodesIterator it( nList );
	while ( it.hasNext() )
		it.next()->setPen(QPen(c), QPen(s));
}

FigureEditor* Triangle::editor() const
{
	return canvas;
}

/**
 * This one moves the triangle/transform, and then notifies the FigureEditer.
 * The FigureEditor should use the moveBy() function to move a triangle
 * instead of this one.
 */
void Triangle::moveTransformBy(double dx, double dy)
{
	TriangleNodesIterator n_iter( nList );
    NodeItem *node;
	QPointF dn(dx, dy);
	int n = 0;
    while ( n_iter.hasNext() )
	{
		node = n_iter.next();
		QPointF& p = cList[n];
		p += dn;
		QPointF npos = mapToItem(node, p);
        node->movePoint(npos.x(), npos.y());
		n++;
    }
	QGraphicsPolygonItem::setPolygon(cList);
	adjustSceneRect();
	coordsToXForm();
}



void Triangle::adjustSceneRect()
{
	if (scene())
	{
		QRectF r = mapToScene(boundingRect()).boundingRect();
		QRectF scene_r = scene()->sceneRect();
		if (!scene_r.contains(r))
			scene()->setSceneRect(scene_r.united(r));
	}
}

NodeItem* Triangle::getNode(int idx) const
{
	return nList[idx];
}

void Triangle::setXform(flam3_xform* xform)
{
	m_xform = xform;
}

// Find the edge closest to the given point.
void Triangle::findEdge(QPointF pos)
{
	QPolygonF poly(polygon());
	QList<QLineF> lines;
	lines << QLineF(poly[0], poly[1])
		<< QLineF(poly[0], poly[2])
		<< QLineF(poly[1], poly[2]);
	QPointF scenePos( mapToScene( pos ) );
	int n(0);
	foreach (QLineF line, lines)
	{
		// determine the minimum distance between pos and each of the edges
		QPointF n0( line.p1() );
		QPointF n1( line.p2() );
		double m( (n1.y() - n0.y()) / (n1.x() - n0.x()) );
		double b( n0.y() -  m * n0.x() );
		double x( (pos.x() + m * (pos.y() - b)) / (m*m + 1 ) );
		double y( m * x + b );
		QPointF scenePoint( mapToScene( x, y ) );
		double dx( scenePoint.x() - scenePos.x() );
		double dy( scenePoint.y() - scenePos.y() );
		double d( sqrt( dx*dx + dy*dy ) );
		// choose the edge that is <2 pixels from pos
		if (d < 2.0)
		{
			logFiner(QString("Triangle::findEdge : found triangle %1 edge at (%2,%3)")
				.arg(m_index).arg(x).arg(y));
			m_edgeLine.setPoints(n0, n1);
			if (n < 2)
				m_edgeType = RotateEdge;
			else
				m_edgeType = ScaleEdge;
			break;
		}
		n++;
	}
	if (m_edgeLine.isNull())
		m_edgeType = NoEdge;
	canvas->update();
}

void Triangle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QGraphicsPolygonItem::paint(painter, option, widget);
	if (canvas->getSelectedTriangle() == this || type() == PostTriangle::RTTI)
	{
		if (!m_edgeLine.isNull())
		{
			// highlight the edge closest to the mouse position found in findEdge()
			QRectF r( mapRectFromScene(0., 0., 2., 2.) );
			qreal width( sqrt( r.width()*r.width() + r.height()*r.height()) );
			logFiner(QString("Triangle::paint : painting selected edge width %1").arg(width));
			painter->save();
			QPen pen(painter->pen());
			pen.setWidthF(width);
			pen.setCapStyle(Qt::RoundCap);
			painter->setPen(pen);
			painter->drawLine(m_edgeLine);
			painter->restore();
			m_edgeLine.setLine(0.,0.,0.,0.);
		}
	}
}

Triangle::EdgeType Triangle::getEdgeType()
{
	return m_edgeType;
}