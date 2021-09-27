#include <d2d1.h>

#include <vector>
#include "Vector2D.h"
using namespace std;

class HullMath {


public: 

	// Check if a point is on an edge
	static bool onLine(D2D1_ELLIPSE end_1, D2D1_ELLIPSE end_2, D2D1_ELLIPSE point) {
		if (end_1.point.x <= max(point.point.x, end_2.point.x) && end_1.point.x >= min(point.point.x, end_2.point.x) && end_1.point.y <= max(point.point.y, end_2.point.y) && end_1.point.y >= min(point.point.y, end_2.point.y)) {
			return true;
		}
		return false;
	}

	static int PointOri(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p3) {
		int value = (p2.point.y - p1.point.y) * (p3.point.x - p2.point.x) - (p2.point.x - p1.point.x) * (p3.point.y - p2.point.y);

		if (value == 0) {
			return 0;
		}
		return (value > 0) ? 1 : 2;
	}

	static int PointDistance(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2) {
		return (p1.point.x - p2.point.x) * (p1.point.x - p2.point.x) - (p1.point.y - p2.point.y) * (p1.point.y - p2.point.y);
	}
	// Determine if a point is to the left of an edge using a cross product!
	// end_1 and end_2 are the endpoints of the edge (going counterclockwise, ideally)
	static bool isLeft(D2D1_ELLIPSE end_1, D2D1_ELLIPSE end_2, D2D1_ELLIPSE point) {
		return ((end_2.point.x - end_1.point.x) * (point.point.y - end_1.point.y) - (end_2.point.y - end_1.point.y) * (point.point.x - end_1.point.x)) > 0;
	}


	static bool LineIntersects(D2D1_ELLIPSE end_11, D2D1_ELLIPSE end_12, D2D1_ELLIPSE end_21, D2D1_ELLIPSE end_22) {
		int o1 = PointOri(end_11, end_12, end_21);
		int o2 = PointOri(end_11, end_12, end_22);
		int o3 = PointOri(end_21, end_22, end_11);
		int o4 = PointOri(end_21, end_22, end_12);

		if (o1 != o2 && o3 != o4) {
			return true;
		}

		if (o1 == 0 && onLine(end_11, end_21, end_12)) {
			return true;
		}

		if (o2 == 0 && onLine(end_11, end_22, end_12)) {
			return true;
		}

		if (o3 == 0 && onLine(end_21, end_11, end_22)) {
			return true;
		}

		if (o4 == 0 && onLine(end_21, end_12, end_22)) {
			return true;
		}

		return false;
	}
	/* IMPORTANT -- Will please read!!
	We will not be implementing the GJK algorithm as a separate method in this class.
	This is because GJK is merely a combination of MinkowskiDiff and HullsIntersecting, which can easily be called in main.cpp.
	What we are mainly concerned with is the recoloring of the hulls based on whether or not they are intersecting, 
	which can ONLY be done in the rendering of the window itself (thus, in main.cpp).

	I have included the rest of the method headers here with comments on their usage to ease our development workflow.
	*/
	
	/* 
	Since the rendering window starts with the top left corner at (0,0), we must manually "move" the working window.
	This means we will translate all points of the hull purely for mathematical purposes (allowing for negative coordinates).

	Depending on the direction (int dir, which will either be -1 or 1), we will either subtract or add to shift the center of our viewing window.
	TranslateHull must thus be called for every algorithm that requires the use of four separate Cartesian quadrants.
	*/
	static void TranslateHull(vector<D2D1_ELLIPSE> hull, int dir) {

		// 500 is a temporary value. We will figure out how to access the window centers later.
		int center_x = 500 * dir;
		int center_y = 500 * dir;

		for each (D2D1_ELLIPSE var in hull) {
			var.point.x = var.point.x + center_x;
			var.point.y = var.point.y + center_y;
		}
	}

	/* ContainsPoint will receive a point and determine if it is inside the given hull. 
	The algorithm will likely be borrowed from prof. McKenna's AFG.

	Translating McKenna et al.'s algorithm is proving to be harder than initially thought. We will do this another way:

	1. Since we are coming into this with a convex hull already computed, we can use vectors to determine our answer.
	2. Go through each edge of the hull counterclockwise. 
	3. For each edge, check to see if the point in question is to the LEFT of the edge.
	4. If it is not for any edge, the point is NOT in the hull.
	5. If it makes a full round with no issues, the point must be in the hull.
	*/
	static bool ContainsPoint(vector<D2D1_ELLIPSE> hull, D2D1_ELLIPSE point) {
		D2D1_ELLIPSE extreme = D2D1::Ellipse(D2D1::Point2F(10000, point.point.y), 10.0f, 10.0f);

		int index = 0;
		int i = 0;

		do {
			int next = (i + 1) % hull.size();

			if (LineIntersects(hull[i], hull[next], point, extreme)) {
				if (PointOri(hull[i], point, hull[next]) == 0) {
					return onLine(hull[i], point, hull[next]);
				}
				index++;
			}
			i = next;
		} while (i != 0);

		return (index % 2) == 1;
	}

	/* HullsIntersecting will probably check if any of the points of hull1 are inside hull2. 
	*  i.e. Loop through all the points of hull1, and for each point, check if it is inside hull2.
	*  Do the same for hull2 and hull1. 
	*  By definition, they cannot intersect without one of the points being inside the other hull.
	*/
	static bool HullsIntersecting(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		for (int i = 0; i < hull1.size(); i++) {
			for (int j = 0; j < hull2.size(); j++) {
				if (LineIntersects(hull1[i], hull1[(i + 1) % hull1.size()], hull2[j], hull2[(j + 1) % hull2.size()])) {
					return true;
				}
			}
		}
		return false;
	}

	/* Algorithm will be borrowed from prof. McKenna's AFG.
	This method must call TranslateHull four times (twice to move the center to the middle of the screen, and twice to return it to the corner).
	*/
	static vector<D2D1_ELLIPSE> MinkowskiSum(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		vector<D2D1_ELLIPSE> sum;
		for (int i = 0; i < hull1.size(); i++) {
			for (int j = 0; j < hull2.size(); j++) {
				sum.push_back(D2D1::Ellipse(D2D1::Point2F(hull1[i].point.x + hull2[j].point.x, hull1[i].point.y + hull2[j].point.y), 10.0f, 10.0f));
			}
		}
		return sum;
	}

	/* Algorithm will be borrowed from prof. McKenna's AFG.
	This method must call TranslateHull four times (twice to move the center to the middle of the screen, and twice to return it to the corner).
	*/
	static vector<D2D1_ELLIPSE> MinkowskiDiff(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		vector<D2D1_ELLIPSE> diff;
		for (int i = 0; i < hull1.size(); i++) {
			for (int j = 0; j < hull2.size(); j++) {
				diff.push_back(D2D1::Ellipse(D2D1::Point2F(hull1[i].point.x - hull2[j].point.x, hull1[i].point.y - hull2[j].point.y), 10.0f, 10.0f));
			}
		}
		return diff;
	}
};