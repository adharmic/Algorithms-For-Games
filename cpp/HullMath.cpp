#include <d2d1.h>

#include <vector>
#include "Vector2D.h"
using namespace std;

class HullMath {

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
		return false;
	}

	/* HullsIntersecting will probably check if any of the points of hull1 are inside hull2. 
	*  i.e. Loop through all the points of hull1, and for each point, check if it is inside hull2.
	*  Do the same for hull2 and hull1. 
	*  By definition, they cannot intersect without one of the points being inside the other hull.
	*/
	static bool HullsIntersecting(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		return false;
	}

	/* Algorithm will be borrowed from prof. McKenna's AFG.
	This method must call TranslateHull four times (twice to move the center to the middle of the screen, and twice to return it to the corner).
	*/
	static vector<D2D1_ELLIPSE> MinkowskiSum(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		return {};
	}

	/* Algorithm will be borrowed from prof. McKenna's AFG.
	This method must call TranslateHull four times (twice to move the center to the middle of the screen, and twice to return it to the corner).
	*/
	static vector<D2D1_ELLIPSE> MinkowskiDiff(vector<D2D1_ELLIPSE> hull1, vector<D2D1_ELLIPSE> hull2) {
		return {};
	}
};