package com.ezsong.commander {
	import com.ruochi.events.Eventer;
	import com.ruochi.text.StyleText;
	import com.ruochi.utils.DelayCall;
	import flash.display.Sprite;
	import flash.external.ExternalInterface;
	import flash.net.LocalConnection;
	import flash.events.StatusEvent;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.net.URLRequest;
	import flash.system.fscommand;
	import flash.net.navigateToURL;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	public class Commander extends Sprite{
		private var _localConnection:LocalConnection = new LocalConnection();
		private var _clientId:String;
		private var _playPauseText:StyleText = new StyleText();
		private var _nextText:StyleText = new StyleText();
		private var _infoText:StyleText = new StyleText();
		private var _volumeText:StyleText = new StyleText();
		private var _delayCall:DelayCall;
		private var _isWorking:Boolean;
		private var _commanderPingTimer:Timer = new Timer(1000);
		public static const PLAY_EVENT:String = "playerEvent";
		
		/*commad*/
		public static const REGISTER_CLIENT:String = "registerClient";
		
		
		
		public function Commander() {
			if(stage){
				init();
			}else {
				addEventListener(Event.ADDED_TO_STAGE, onAddToStage);
			}
			_clientId = "_1g1gClient_"+Math.random();
			_localConnection = new LocalConnection();
			_localConnection.allowDomain("*");
			_localConnection.allowInsecureDomain("*");
			_localConnection.connect(_clientId);
			_localConnection.client = this;
			_localConnection.addEventListener(StatusEvent.STATUS, onLocalConnectionStatus);
			if (ExternalInterface.available) {
				ExternalInterface.addCallback("command", command);
			}			
			_localConnection.send("_1g1g_player", "command", "registerClient", _clientId);
		}
		
		private function onAddToStage(e:Event):void {
			removeEventListener(Event.ADDED_TO_STAGE, onAddToStage); 
			init();
		}
		
		
		
		private function init():void {			
			var gapX:int = 5;
			_playPauseText.text = "播放";		
			_nextText.text = "下一首";		
			_volumeText.text = "打开声音";	
			_infoText.text = "info";
			_nextText.x = _playPauseText.x + _playPauseText.width + gapX;
			_volumeText.x = _nextText.x + _nextText.width + gapX;
			_infoText.x = _volumeText.x + _volumeText.width + gapX;
			_playPauseText.addEventListener(MouseEvent.CLICK, onPlayPauseTextClick);
			_nextText.addEventListener(MouseEvent.CLICK, onNextTextClick);
			_commanderPingTimer.addEventListener(TimerEvent.TIMER, onCommanderPingTimer);
			_volumeText.addEventListener(MouseEvent.CLICK, onVolumeTextClick);	
			_infoText.addEventListener(MouseEvent.CLICK, onInfoTextClick);
			addChild(_playPauseText);
			addChild(_nextText);
			addChild(_volumeText);
			addChild(_infoText);
			_commanderPingTimer.start();
		}
		
		private function onCommanderPingTimer(e:TimerEvent):void {
			command("registerClient");
		}
		
		private function onInfoTextClick(e:MouseEvent):void {
			
		}
		
		private function onVolumeTextClick(e:MouseEvent):void {
			command("volumeOnOff");
		}
		
		private function onNextTextClick(e:MouseEvent):void {
			command("next");
		}
		
		private function onPlayPauseTextClick(e:MouseEvent):void {
			command("playPause");
		}
		
		private function onLocalConnectionStatus(e:StatusEvent):void {
			
		}
		
		public function playerEventDispatcher(type:String, body:Object):void {
			if (_isWorking == false)
			{
				_isWorking = true;
				ExternalInterface.call("playerEventDispatcher", "connect");
				dispatchEvent(new Event(Event.OPEN));
			}
			try
			{
				if (ExternalInterface.available) {
					ExternalInterface.call("playerEventDispatcher", type, body);
				}
				dispatchEvent(new Eventer(PLAY_EVENT,{type:type, body:body}));
				if (type == "playheadUpdate") {
					_infoText.text = body.currentTime + "/" + body.totalTime;
				}
				if (type == "playerStateChange") {
					_infoText.text = body.state
					if (body.state == "playing" || body.state == "loading") {
						_playPauseText.text = "暂停";
					}else if (body.state == "pause" || body.state == "stop") {
						_playPauseText.text = "播放";
					}
				}
				if (type == "currentSongInfo") {
					_infoText.text = body.name + "-" + body.singer;
				}
				if (type == "lyricSentence") {
					_infoText.text = body.lyricSentence
				}
				
			}
			catch (e:Error)
			{
				
			}
			finally
			{
				_commanderPingTimer.stop();
				if (_delayCall) {
					_delayCall.stop();
				}
				_delayCall = DelayCall.call(2, disconnect);
			}
		}
		
		private function disconnect():void
		{
			_isWorking = false;
			_commanderPingTimer.start(); 
			if (ExternalInterface.available) {
				ExternalInterface.call("playerEventDispatcher", "disconnect");
			}
			dispatchEvent(new Event(Event.CLOSE));
		}
		
		public function command(name:String, body:Object = null):Object {
			var localConnection:LocalConnection = new LocalConnection();
			if (name == "is1g1gPlayerOpen") {
				try {
					localConnection.connect("_1g1g_player");
					localConnection.close();
					return false;
				}catch (e:Error) {
					return true;
				}
			}
			if (name == "play") {
				try {
					localConnection.connect("_1g1g_player");
					localConnection.close();
					navigateToURL(new URLRequest("http://www.1g1g.com/" + encodeURI(String(body))), "_blank");
				}catch (e:Error) {
					send(name, body);
				}
			}else{			
				send(name, body);
			}
			return null;
		}
		
		private function send(name:String, body:Object):void {
			try {					
				_localConnection.send("_1g1g_player", "command", name, body);
				_localConnection.send("_1g1g_player", "command", "registerClient", _clientId);
			}catch (e:Error) {
				
			}
		}
	}
}