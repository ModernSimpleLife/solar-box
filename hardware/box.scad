$fn = 50;
in_width = 12;
in_height= 32;
in_depth = 8;
wall_thickness = 1;
out_width = in_width + 2 * wall_thickness;
out_height = in_height + 2 * wall_thickness;
out_depth = in_depth + 2 * wall_thickness;
mounting_hole_diameter = 2;
mounting_hole_size = 4;

module lid_mount(x, y) {
	translate([x, y, 0]) {
		difference() {
			cube([mounting_hole_size, mounting_hole_size, wall_thickness], center=true);
			cylinder(2*wall_thickness, mounting_hole_diameter / 2, center=true);
		}
	}
}

module bottom() {
	difference() {
		// outer
		cube([out_width, out_height, out_depth], center=true);

		// inner
		translate([0, 0, wall_thickness]) {
			cube([in_width, in_height, out_depth], center=true);
		}
	}

	translate([0, 0, (out_depth - wall_thickness) / 2]) {
		lid_mount((out_width + mounting_hole_size) / 2, -out_height / 4);
		lid_mount((out_width + mounting_hole_size) / 2, out_height / 4);
		lid_mount(-(out_width + mounting_hole_size) / 2, -out_height / 4);
		lid_mount(-(out_width + mounting_hole_size) / 2, out_height / 4);
	}
}

module top() {
	translate([0, 0, 0]) {
		cube([out_width, out_height, wall_thickness], center=true);
	}

	lid_mount((out_width + mounting_hole_size) / 2, -out_height / 4);
	lid_mount((out_width + mounting_hole_size) / 2, out_height / 4);
	lid_mount(-(out_width + mounting_hole_size) / 2, -out_height / 4);
	lid_mount(-(out_width + mounting_hole_size) / 2, out_height / 4);
}

bottom();

translate([0, 0, (out_depth + 4 * wall_thickness) / 2]) {
	top();
}
