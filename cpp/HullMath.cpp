#include <d2d1.h>

#include <vector>
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

	}

	/* ContainsPoint will receive a point and determine if it is inside the given hull. 
	The algorithm will likely be borrowed from prof. McKenna's AFG.
	*/
	static bool ContainsPoint(vector<D2D1_ELLIPSE> hull, D2D1_ELLIPSE point) {
		return false;
	}

	/* HullsIntersecting will probably check if any of the points of hull1 are inside hull2. */
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