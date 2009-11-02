package com.ruochi.graphic{
	import flash.display.Shape;
	public function drawArc(sp:Shape,nX:Number, nY:Number, nRadius:Number, nArc:Number, nStartingAngle:Number = 0):void {
		var arcArray:Array = new Array();
		var ctrlDistArray:Array = new Array();
		var direct:int;
		nStartingAngle *= Math.PI / 180;
		var nAngle:Number = nStartingAngle;
		var nCtrlX:Number;
		var nCtrlY:Number;
		var nAnchorX:Number;
		var nAnchorY:Number;
		if (nArc<0) {
			direct = -1;
			nArc = -nArc;
		} else {
			direct = 1;
		}
		while (nArc>45) {
			arcArray.push(45*direct*Math.PI/180);
			nArc = nArc - 45;
		}
		if (nArc!=0) {
			arcArray.push(nArc*direct*Math.PI/180);
		}
		for (var i:Number = 0; i < arcArray.length; i++) {
			ctrlDistArray.push( nRadius/Math.cos(arcArray[i]/2));
		}		
		var nStartingX:Number = nX + Math.cos(nStartingAngle) * nRadius;
		var nStartingY:Number = -(nY + Math.sin(nStartingAngle) * nRadius);
		for (i = 0; i < arcArray.length; i++) {
			nAngle += arcArray[i];
			nCtrlX = nX + Math.cos(nAngle-(arcArray[i]/2))*(ctrlDistArray[i]);
			nCtrlY = nY + Math.sin(nAngle-(arcArray[i]/2))*(ctrlDistArray[i]);
			nAnchorX = nX + Math.cos(nAngle) * nRadius;
			nAnchorY = nY + Math.sin(nAngle) * nRadius;
			sp.graphics.curveTo(nCtrlX, -nCtrlY, nAnchorX, -nAnchorY);
		}
	}
}