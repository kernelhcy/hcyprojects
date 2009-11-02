package com.ezsong.controller 
{
	import com.ezsong.shape.NextShape;
	import com.ruochi.text.StyleText;
	import flash.display.Sprite;
	import com.ruochi.component.ShapeGlowBtn;
	import com.ezsong.shape.PlayShape;
	import com.ezsong.shape.PauseShape;
	import com.ezsong.commander.Commander;
	import flash.events.MouseEvent;
	import flash.system.Security;
	import flash.display.StageScaleMode;
	import flash.display.StageAlign;
	import com.ruochi.display.ElementContainer;
	import com.ruochi.layout.Margin;
	import com.ruochi.layout.LayoutUtils;
	import flash.events.Event;
	import flash.display.Stage;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	import com.ezsong.player.common.StatusType;
	import com.ezsong.player.common.PublicCommands;
	import com.ezsong.player.common.PlayerStateType;
	public class Controller extends ElementContainer
	{
		private var _playPauseBtn:ShapeGlowBtn = new ShapeGlowBtn(36, 40, new PlayShape(20,2));
		private var _nextBtn:ShapeGlowBtn = new ShapeGlowBtn(30, 40, new NextShape(10,1));
		private var _infoText:StyleText = new StyleText();
		private var _margin:Margin = new Margin("5");
		private var _commander:Commander = new Commander();
		private var _commanderPingTimer:Timer = new Timer(500);
		public function Controller() 
		{
			if(stage){
				init();
			}else {
				addEventListener(Event.ADDED_TO_STAGE, onAddToStage);
			}
		}
		
		
		private function onAddToStage(e:Event):void {
			removeEventListener(Event.ADDED_TO_STAGE, onAddToStage); 
			init();
		}
		private function init():void
		{
			Security.allowDomain("*");
			if(parent is Stage){
				stage.scaleMode = StageScaleMode.NO_SCALE;		
				stage.align = StageAlign.TOP_LEFT;
				stage.addEventListener(Event.RESIZE, onStageResize);
				//stage.stageFocusRect = false;				
			}
			setChildren();
			addChildren();
			addListener();
			setLayout();
		}
		
		private function onStageResize(e:Event):void 
		{
			setLayout();
		}
		private function setChildren():void 
		{
			_playPauseBtn.x = _margin.left.value;
			_playPauseBtn.icon = new PauseShape(20,2);
			LayoutUtils.alginRight(_nextBtn, _playPauseBtn, 5);
			_infoText.text = "loading adsf asf sadf adsf adsf afdas fasfd asfd ";
			_infoText.background = true;			
			_commanderPingTimer.start();
		}
		private function addChildren():void 
		{
			addChild(_playPauseBtn);
			addChild(_nextBtn);
			addChild(_infoText);
		}
		private function addListener():void 
		{
			_commander.addEventListener(Commander.PLAY_EVENT, onCommanderPlayerEvent);
			_commanderPingTimer.addEventListener(TimerEvent.TIMER, onCommanderPingTimer);
			_playPauseBtn.addEventListener(MouseEvent.CLICK, onPlayPauseBtnClick);
			_nextBtn.addEventListener(MouseEvent.CLICK, onNextBtnClick);
		}
		
		private function onPlayPauseBtnClick(e:MouseEvent):void 
		{
			_commander.command(PublicCommands.PLAY_PAUSE);
			
		}
		
		private function onNextBtnClick(e:MouseEvent):void 
		{
			_commander.command(PublicCommands.NEXT);
		}
		
		private function onCommanderPingTimer(e:TimerEvent):void 
		{
			_commander.command(Commander.REGISTER_CLIENT);
		}
		
		private function onCommanderPlayerEvent(e:Event):void 
		{
			var type:String = e.eventInfo.type;
			var body:Object = e.eventInfo.body;
			
			
			try{
				if (type == StatusType.CURRENT_SONG_INFO) {
					_infoText.text = body.name +"-" + body.singer";
					
				}
				else if ( type == StatusType.PLAYER_STATE)
				{
					if (body == PlayerStateType.STOP || body == PlayerStateType.PAUSED) 
					{
						_playPauseBtn.iconWrapper.frame = 2;
					}
					else
					{
						_playPauseBtn.iconWrapper.frame = 1;
					}
				}
			}catch (e:Error) {
				alert(e);
			}
			if (_commanderPingTimer.running) {
				_commanderPingTimer.stop();
			}
		}
		
		override public function set width(value:Number):void {
			_width = value;
			LayoutUtils.horizontalStretch(_infoText,LayoutUtils.getDisplayObjectRightX(_nextBtn) + 5, _margin.right.value);
			
		}
		
		override public function set height(value:Number):void {
			_height = value;
			LayoutUtils.setMiddle(_playPauseBtn);
			LayoutUtils.setMiddle(_nextBtn);
			LayoutUtils.setMiddle(_infoText);
			
		}
		
		private function setLayout():void{
			width = stage.stageWidth;
			height = stage.stageHeight;
		}
	}	
}