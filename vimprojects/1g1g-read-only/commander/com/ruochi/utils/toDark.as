package com.ruochi.utils {
	import com.ruochi.geom.Color;
	public function toDark(col:uint, degree:Number = 0):uint {
		var color:Color = new Color(col);
		color.red = Math.round(color.red * (1 - degree));
		color.green = Math.round(color.green * (1 - degree));
		color.blue = Math.round(color.blue * (1 - degree));
		return color.color;
	}
}