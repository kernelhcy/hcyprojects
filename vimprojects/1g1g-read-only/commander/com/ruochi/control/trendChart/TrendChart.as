package com.ruochi.control.trendChart {
	import flash.display.Sprite;
	import com.ruochi.shape.Circle;
	import com.ruochi.utils.deleteAll;
	public class TrendChart extends Sprite {
		private var _dataXml:XML;
		private var _valueKey:String = "count";
		private var _unitName:String ="paper";
		private var _valueMax:Number = 10;
		private var _valueMin:Number = 0 ;
		private var _dotWidth:Number = 5;
		private var _numColume:int;
		private var _xGap:Number;
		private var _chartWidth:Number;
		private var _chartHeight:Number;
		private var _wrapper:Sprite = new Sprite;
		public function TrendChart(w:Number=100,h:Number=100) {
			_chartWidth = w;
			_chartHeight = h;
			_wrapper.y = h;
			addChild(_wrapper);
		}
		private function init():void {
			initVar();
		}
		private function initVar():void {
			_numColume = _dataXml[_unitName].length();
			_xGap = _chartWidth / _numColume;
		}
		private function buildUI():void {
			deleteAll(_wrapper);
			_wrapper.graphics.clear();
			_wrapper.graphics.lineStyle(4, 0xffffff);
			for (var i:int = 0; i < _numColume; i++) {
				var circle:Circle = new Circle(_dotWidth / 2);
				var xx:Number
				var yy:Number;
				xx = _xGap * i;
				yy = -_chartHeight * (Number(_dataXml[_unitName][i][_valueKey][0]) - _valueMin) / (_valueMax - _valueMin);
				if (i == 0) {
					_wrapper.graphics.moveTo(xx, yy);
				}else {
					_wrapper.graphics.lineTo(xx, yy);
				}
				circle.x = _xGap * i - _dotWidth/2;
				circle.y = -_chartHeight * (Number(_dataXml[_unitName][i][_valueKey][0]) - _valueMin) / (_valueMax - _valueMin) - _dotWidth/2;
				_wrapper.addChild(circle);								
			}
		}
		public function set valueKey(str:String):void {
			_valueKey = str;
		}
		public function set dataXml(xml:XML):void {
			_dataXml = xml;
			initVar();
			buildUI();
		}
	}	
}