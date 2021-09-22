//#include "quickhull.h"
//
//quickhull::quickhull(list<shared_ptr<myellipse>> orig_list) {
//	original_list = orig_list;
//	selection = original_list.begin();
//}
//
//list<myellipse>  quickhull::getconvexhull() {
//	list<myellipse> hull;
//	list<myellipse> extreme_points;
//
//	myellipse left_most_point = **(this -> selection);
//	myellipse right_most_point = **(this->selection);
//	myellipse top_most_point = **(this->selection);
//	myellipse bottom_most_point = **(this->selection);
//
//	for (size_t i = 0; i < original_list.size(); i++) {
//		myellipse current_point = **(this->selection);
//		if (current_point.ellipse.point.x < left_most_point.ellipse.point.x) {
//			left_most_point = current_point;
//		}
//		if (current_point.ellipse.point.x > right_most_point.ellipse.point.x) {
//			right_most_point = current_point;
//		}
//		if (current_point.ellipse.point.y < top_most_point.ellipse.point.y) {
//			top_most_point = current_point;
//		}
//		if (current_point.ellipse.point.y > bottom_most_point.ellipse.point.y) {
//			bottom_most_point = current_point;
//		}
//		advance(selection, 1);
//	}
//
//	hull.push_back(top_most_point);
//	hull.push_back(right_most_point);
//	hull.push_back(bottom_most_point);
//	hull.push_back(left_most_point);
//
//	extreme_points.push_back(top_most_point);
//	extreme_points.push_back(right_most_point);
//	extreme_points.push_back(bottom_most_point);
//	extreme_points.push_back(left_most_point);
//
//	for (size_t i = 1; i < hull.size() + 1; i++) {
//		auto hull_a = std::next(hull.begin(), 0);
//		auto hull_b = std::next(hull.begin(), 0);
//		if (i != hull.size()) {
//			hull_a = std::next(hull.begin(), i - 1);
//			hull_b = std::next(hull.begin(), i);
//		}
//		else {
//			hull_a = std::next(hull.begin(), hull.size() - 1);
//			hull_b = std::next(hull.begin(), 0);
//		}
//		myellipse farthest_pt = pointfarthestfromedge(*hull_a, *hull_b, original_list);
//		if (!contains(hull, farthest_pt)) {
//			hull.insert(hull_b, farthest_pt);
//			if (i < 1) {
//				i = i - 2;
//			}
//			else {
//				i--;
//			}
//		}
//	}
//	return hull;
//
//
//}