package com.ruochi.sound {
	import flash.media.SoundChannel;
	import flash.media.SoundTransform;
	public class SoundChanenlVolume {
		private var _soundChannel:SoundChannel;
		private var _volume:Number;
		public function SoundChanenlVolume(sc:SoundChannel,v:Number =1) {
			_soundChannel = sc;
			_volume = v;
		}
		
		public function get volume():Number { return _volume; }
		
		public function set volume(value:Number):void {
			_volume = value;
			_soundChannel.soundTransform = new SoundTransform(_volume);
		}
		
	}
	
}