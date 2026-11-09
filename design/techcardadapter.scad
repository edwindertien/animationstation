$fn=40;
height = 3;
plate();
//smallplate();
//smallrotor();
//rotor();
module smallrotor(){
difference(){
union(){
    translate([-12.5,0,0])cylinder(d=15,h=height);
    translate([12.5,0,0])cylinder(d=15,h=height);
    translate([-10,-5,0])cube([20,10,height]);
    cylinder(d=8,h=height+3);
}
    translate([-12.5,0,0])cylinder(d=5,h=height);
    translate([12.5,0,0])cylinder(d=5,h=height);

    translate([0,0,height+1])cylinder(d=4.9,h=30);
    cylinder(d=2,h=5);
    translate([0,0,-0.1])cylinder(d=5,h=1.5);
}

}
module plate(){
    difference(){
        union(){
            translate([-25,-12.5,0])cylinder(d=15,h=height);
            translate([0,-12.5,0])cylinder(d=15,h=height);
            translate([25,-12.5,0])cylinder(d=15,h=height);
            translate([-25,12.5,0])cylinder(d=15,h=height);
            translate([0,12.5,0])cylinder(d=15,h=height);
            translate([25,12.5,0])cylinder(d=15,h=height);
            translate([-30,-15,0])cube([60,30,height]);
            translate([-22.5,-10,height])cube([2,20,1.5]);
            translate([20.5,-10,height])cube([2,6,1.5]);
            translate([20.5,4,height])cube([2,6,1.5]);
            translate([-16.5,-12,height])cube([8,2,1]);
            translate([8.5,-12,height])cube([8,2,1]);
            translate([-16.5,10,height])cube([8,2,1]);
            translate([8.5,10,height])cube([8,2,1]);
        }
            translate([-25,-12.5,0])cylinder(d=5,h=height);
            translate([0,-12.5,0])cylinder(d=5,h=height);
            translate([25,-12.5,0])cylinder(d=5,h=height);
            translate([-25,12.5,0])cylinder(d=5,h=height);
            translate([0,12.5,0])cylinder(d=5,h=height);
            translate([25,12.5,0])cylinder(d=5,h=height);
            translate([-20.5,-10,height-0.5])cube([41,20,2]);
            translate([-16,-5,-0.10])cube([32,10,20]);
    }
}

module smallplate(){
    difference(){
        union(){
            translate([-12.5,-12.5,0])cylinder(d=15,h=height);
            translate([12.5,-12.5,0])cylinder(d=15,h=height);
            translate([-12.5,12.5,0])cylinder(d=15,h=height);
            translate([12.5,12.5,0])cylinder(d=15,h=height);
            
            translate([-15,-15,0])cube([30,30,height]);
            
            translate([-13,-6,height])cube([1.5,12,1.5]);
            translate([11.5,-6,height])cube([1.5,4,1.5]);
            translate([11.5,2,height])cube([1.5,4,1.5]);
            
          

        }
            translate([-12.5,-12.5,0])cylinder(d=5,h=height);
            translate([12.5,-12.5,0])cylinder(d=5,h=height);
            translate([-12.5,12.5,0])cylinder(d=5,h=height);
            translate([12.5,12.5,0])cylinder(d=5,h=height);
        
            translate([-11.5,-6,height-0.5])cube([23,12,1]);
            translate([-5,-2,-0.1])cube([10,4,height+1]);
    }
}


module rotor() {difference(){
    union(){
        cylinder(d=50,h=height);
        cylinder(d=10,h=height+4);
        rotate([0,0,0])translate([4,-1,height])rotate([0,11,0])cube([15,2,3]);
        rotate([0,0,90])translate([4,-1,height])rotate([0,11,0])cube([15,2,3]);
        rotate([0,0,180])translate([4,-1,height])rotate([0,11,0])cube([15,2,3]);
        rotate([0,0,270])translate([4,-1,height])rotate([0,11,0])cube([15,2,3]);
    }
    translate([12.5,12.5,-0.1])cylinder(d=5,h=height+1);
    translate([-12.5,12.5,-0.1])cylinder(d=5,h=height+1);
    translate([12.5,-12.5,-0.1])cylinder(d=5,h=height+1);
    translate([-12.5,-12.5,-0.1])cylinder(d=5,h=height+1);
    translate([0,0,height+1])cylinder(d=5.9,h=30);
    cylinder(d=2,h=5);
    translate([0,0,-0.1])cylinder(d=5,h=1.5);
    translate([0,0,-10])cylinder(d=60,h=10);
}

}