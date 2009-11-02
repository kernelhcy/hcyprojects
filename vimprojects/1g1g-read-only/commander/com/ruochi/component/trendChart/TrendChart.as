package com.ruochi.component.trendChart {
	import flash.display.Sprite;
	import com.ruochi.component.trendChart.PointUnit;
	import com.ruochi.utils.deleteAll;
	import flash.events.Event;
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import com.ruochi.utils.xmlToVar;
	import flash.display.StageAlign;
	import flash.display.StageScaleMode;
	import com.ruochi.utils.stringToDate;
	import com.ruochi.utils.stringToNum;
	import com.ruochi.text.StyleText;
	import com.ruochi.layout.setCenter;
	import com.ruochi.component.CircleProgressBar;
	public class TrendChart extends Sprite {
		private var _wrapper:Sprite = new Sprite();
		private var _dataXml:XML;
		private var _styleText:StyleText = new StyleText();
		private var _circleProgressBar:CircleProgressBar = new CircleProgressBar;
		public function TrendChart() {
			addEventListener(Event.ADDED_TO_STAGE, onAddToStage, false, 0, true);
			stage.scaleMode = StageScaleMode.NO_SCALE;
			stage.align = StageAlign.TOP_LEFT;
		}		
		private function onAddToStage(e:Event):void {
			init();
			stage.align = StageAlign.TOP_LEFT;
			stage.scaleMode = StageScaleMode.NO_SCALE;
		}
		private function setChartDimention():void {
			TrendChartConfig.chartHeight = stage.stageHeight - TrendChartConfig.marginLeft - TrendChartConfig.marginRight;
			TrendChartConfig.chartWidth = stage.stageWidth - TrendChartConfig.marginLeft - TrendChartConfig.marginRight;
			_wrapper.x = TrendChartConfig.marginLeft;
			_wrapper.y = TrendChartConfig.marginTop;;
		}
		private function init():void {
			stage.addChild(_circleProgressBar);
			_circleProgressBar.loaderColor = 0xff6600;
			_circleProgressBar.x = stage.stageWidth / 2;
			_circleProgressBar.y = stage.stageHeight / 2;
			_circleProgressBar.start();
			initVar();
		}
		private function initVar():void {
			var urlLoader:URLLoader = new URLLoader();
			var url:String = loaderInfo.parameters["xml"]?loaderInfo.parameters["xml"]:"data.xml";
			urlLoader.addEventListener(Event.COMPLETE, onUrlLoaderComplete, false, 0, true);
			urlLoader.load(new URLRequest(url));
		}		
		private function onUrlLoaderComplete(e:Event):void {
			_dataXml = new XML(e.target.data);
			if (_dataXml.error[0]) {
				_styleText.text = _dataXml.error[0];
				_styleText.color = 0;
				addChild(_styleText);
				setCenter(_styleText,stage);
			}else {
				if (_dataXml.line.length() > 0) {
					formatXml(_dataXml);
					xmlToVar(_dataXml.config[0], TrendChartConfig);
					if (isNaN(TrendChartConfig.xMax) || isNaN(TrendChartConfig.xMin) || isNaN(TrendChartConfig.yMax) || isNaN(TrendChartConfig.yMin)) {
						getMaxMin(_dataXml);
					}
					run();
				} else {
					_styleText.text = "No data available";
					_styleText.color = 0;
					addChild(_styleText);
					setCenter(_styleText,stage);
				}
			}
			_circleProgressBar.stop();
		}
		private function configListener():void {
			stage.addEventListener(Event.RESIZE, onStageResize, false, 0, true);
		}
		
		private function onStageResize(e:Event):void {
			setChartDimention();
			buildUI();
		}
		private function run():void {			
			setChartDimention();
			addChild(_wrapper);
			configListener()
			buildUI();
		}
		private function buildUI():void {
			deleteAll(_wrapper);
			_wrapper.graphics.clear();
			drawBgLine();
			_wrapper.graphics.lineStyle(4, 0xff6600);
			var lineLength:int = _dataXml.line.length();
			for (var i:int = 0; i < lineLength; i++) {
				drawTrendLine(_dataXml.line[i]);
			}
		}
		private function drawTrendLine(xml:XML):void {
			var length:int = xml.point.length();
			_wrapper.graphics.lineStyle(2, xml.config[0].color[0]);
			_wrapper.graphics.moveTo((xml.point[0].xValue[0] - TrendChartConfig.xMin) / (TrendChartConfig.xMax - TrendChartConfig.xMin) * TrendChartConfig.chartWidth, TrendChartConfig.chartHeight -(xml.point[0].yValue[0] - TrendChartConfig.yMin) / (TrendChartConfig.yMax - TrendChartConfig.yMin) * TrendChartConfig.chartHeight);
			for (var i:int = 0; i < length; i++) {				
				var pointUnit:PointUnit = new PointUnit();
				pointUnit.dataXml = xml.point[i];
				_wrapper.graphics.lineTo(pointUnit.x, pointUnit.y);
				_wrapper.addChild(pointUnit);
			}
		}
		private function formatXml(xml):void {
			var lineLength:int = xml.line.length();
			var i:int;
			for (i = 0; i < lineLength; i++) {
				var pointLength:int = xml.line[i].point.length();
				for (var j:int = 0; j < pointLength; j++) {
					var px:String = xml.line[i].point[j].x[0];
					var py:String = xml.line[i].point[j].y[0];
					xml.line[i].point[j].xValue[0] = stringToNum(px);
					xml.line[i].point[j].yValue[0] = stringToNum(py);
				}
			}
		}
		private function drawBgLine():void {
			_wrapper.graphics.lineStyle(1, 0xcccccc);
			var length:int;
			var i:int;
			length = _dataXml.config[0].vLineValue.length();			
			for (i = 0; i < length; i++) {
				_wrapper.graphics.moveTo((stringToNum(_dataXml.config[0].vLineValue[i]) - TrendChartConfig.xMin) / (TrendChartConfig.xMax - TrendChartConfig.xMin) * TrendChartConfig.chartWidth, 0);
				_wrapper.graphics.lineTo((stringToNum(_dataXml.config[0].vLineValue[i]) - TrendChartConfig.xMin) / (TrendChartConfig.xMax - TrendChartConfig.xMin) * TrendChartConfig.chartWidth, TrendChartConfig.chartHeight);
				if(TrendChartConfig.isShowLabel){
					var styleText:StyleText = new StyleText();
					styleText.color = 0;
					styleText.align = "left";
					styleText.text = _dataXml.config[0].vLineValue[i];
					styleText.y = TrendChartConfig.chartHeight;
					styleText.x = (stringToNum(_dataXml.config[0].vLineValue[i]) - TrendChartConfig.xMin) / (TrendChartConfig.xMax - TrendChartConfig.xMin) * TrendChartConfig.chartWidth - styleText.width/2
					_wrapper.addChild(styleText);
				}
			}
			
			length = _dataXml.config[0].hLineValue.length();
			for (i = 0; i < length; i++) {
				_wrapper.graphics.moveTo(0,TrendChartConfig.chartHeight - (stringToNum(_dataXml.config[0].hLineValue[i]) - TrendChartConfig.yMin) / (TrendChartConfig.yMax - TrendChartConfig.yMin) * TrendChartConfig.chartHeight);
				_wrapper.graphics.lineTo(TrendChartConfig.chartWidth, TrendChartConfig.chartHeight - (stringToNum(_dataXml.config[0].hLineValue[i]) - TrendChartConfig.yMin) / (TrendChartConfig.yMax - TrendChartConfig.yMin) * TrendChartConfig.chartHeight);
				if(TrendChartConfig.isShowLabel){
					styleText = new StyleText();
					styleText.color = 0;
					styleText.align = "left";
					styleText.text = _dataXml.config[0].hLineValue[i];
					styleText.y = TrendChartConfig.chartHeight - (stringToNum(_dataXml.config[0].hLineValue[i]) - TrendChartConfig.yMin) / (TrendChartConfig.yMax - TrendChartConfig.yMin) * TrendChartConfig.chartHeight - styleText.height;
					_wrapper.addChild(styleText);
				}
			}
		}
		private function getMaxMin(xml):void {
			var xMax:Number = Number.NEGATIVE_INFINITY;
			var yMax:Number = Number.NEGATIVE_INFINITY;
			var xMin:Number = Number.POSITIVE_INFINITY;
			var yMin:Number = Number.POSITIVE_INFINITY;
			var lineLength:int = xml.line.length();
			for (var i:int = 0; i < lineLength; i++) {
				var pointLength:int = xml.line[i].point.length();
				for (var j:int = 0; j < pointLength; j++) {
					var px:Number = xml.line[i].point[j].xValue[0];
					var py:Number = xml.line[i].point[j].yValue[0];
					if (xMax < px) {
						xMax = px;
					}
					if (yMax < py) {
						yMax = py;
					}
					if (xMin > px) {
						xMin = px
					}
					if (yMin > py) {
						yMin = py
					}
				}
			}
			TrendChartConfig.xMax = xMax;
			TrendChartConfig.xMin = xMin;
			TrendChartConfig.yMax = yMax;
			TrendChartConfig.yMin = yMin;
		}
	}	
}