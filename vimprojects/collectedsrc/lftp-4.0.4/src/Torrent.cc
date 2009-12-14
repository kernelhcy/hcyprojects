/*
 * lftp and utils
 *
 * Copyright (c) 2009 by Alexander V. Lukyanov (lav@yars.free.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: Torrent.cc,v 1.28 2009/09/23 08:19:41 lav Exp $ */

#include <config.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sha1.h>

#include "Torrent.h"
#include "log.h"
#include "url.h"
#include "misc.h"

static ResType torrent_vars[] = {
   {"torrent:port-range", "6881-6889", ResMgr::RangeValidate, ResMgr::NoClosure},
   {"torrent:max-peers", "60", ResMgr::UNumberValidate},
   {"torrent:stop-on-ratio", "2.0", ResMgr::FloatValidate},
   {"torrent:seed-max-time", "30d", ResMgr::TimeIntervalValidate},
   {"torrent:seed-min-peers", "3", ResMgr::UNumberValidate},
   {"torrent:ip", "", ResMgr::IPv4AddrValidate, ResMgr::NoClosure},
   {0}
};
static ResDecls torrent_vars_register(torrent_vars);

xstring Torrent::my_peer_id;
xstring Torrent::my_key;
Ref<TorrentListener> Torrent::listener;
Ref<FDCache> Torrent::fd_cache;

Torrent::Torrent(const char *mf,const char *c,const char *od)
   : metainfo_url(mf),
     tracker_timer(600), pieces_needed_rebuild_timer(10),
     cwd(c), output_dir(od), rate_limit(mf),
     seed_timer("torrent:seed-max-time",0),
     optimistic_unchoke_timer(30), peers_scan_timer(1),
     am_interested_timer(1)
{
   started=false;
   shutting_down=false;
   complete=false;
   end_game=false;
   validating=false;
   force_valid=false;
   validate_index=0;
   pieces=0;
   name=0;
   piece_length=0;
   last_piece_length=0;
   total_length=0;
   total_sent=0;
   total_recv=0;
   total_left=0;
   complete_pieces=0;
   active_peers_count=0;
   complete_peers_count=0;
   am_interested_peers_count=0;
   am_not_choking_peers_count=0;
   max_peers=60;
   seed_min_peers=3;
   stop_on_ratio=2;
   last_piece=TorrentPeer::NO_PIECE;
   Reconfig(0);

   if(listener==0) {
      listener=new TorrentListener();
      fd_cache=new FDCache();
   }
   if(!my_peer_id) {
      my_peer_id.set("-lftp40-");
      my_peer_id.appendf("%04x",(unsigned)getpid());
      my_peer_id.appendf("%08x",(unsigned)now.UnixTime());
      assert(my_peer_id.length()==PEER_ID_LEN);
   }
   if(!my_key) {
      for(int i=0; i<10; i++)
	 my_key.appendf("%02x",unsigned(random()/13%256));
   }
}

int Torrent::Done()
{
   return (shutting_down && !tracker_reply);
}

void Torrent::Shutdown()
{
   if(shutting_down)
      return;
   LogNote(3,"Shutting down...");
   shutting_down=true;
   if(listener)
      listener->RemoveTorrent(this);
   if(started || tracker_reply)
      SendTrackerRequest("stopped");
   if(listener && listener->GetTorrentsCount()==0) {
      listener=0;
      fd_cache=0;
   }
   peers.unset();
}

void Torrent::PrepareToDie()
{
   peers.unset();
   if(listener)
      listener->RemoveTorrent(this);
   if(listener && listener->GetTorrentsCount()==0) {
      listener=0;
      fd_cache=0;
   }
}

void Torrent::SetError(Error *e)
{
   if(invalid_cause)
      return;
   invalid_cause=e;
   LogError(0,"%s: %s",
      invalid_cause->IsFatal()?"Fatal error":"Transient error",
      invalid_cause->Text());
   tracker_reply=0;
   Shutdown();
}
void Torrent::SetError(const char *msg)
{
   SetError(Error::Fatal(msg));
}

double Torrent::GetRatio() const
{
   if(total_sent==0 || total_length==total_left)
      return 0;
   return total_sent/double(total_length-total_left);
}

void Torrent::SetDownloader(unsigned piece,unsigned block,const TorrentPeer *o,const TorrentPeer *n)
{
   const TorrentPeer*& downloader=piece_info[piece]->downloader[block];
   if(downloader==o)
      downloader=n;
}

BeNode *Torrent::Lookup(xmap_p<BeNode>& dict,const char *name,BeNode::be_type_t type)
{
   BeNode *node=dict.lookup(name);
   if(!node) {
      SetError(xstring::format("Meta-data: `%s' key missing",name));
      return 0;
   }
   if(node->type!=type) {
      SetError(xstring::format("Meta-data: wrong `%s' type, must be %s",name,BeNode::TypeName(type)));
      return 0;
   }
   return node;
}
void Torrent::InitTranslation()
{
   const char *charset="UTF-8"; // default
   BeNode *b_charset=metainfo_tree->dict.lookup("encoding");
   if(b_charset && b_charset->type==BeNode::BE_STR)
      charset=b_charset->str;
   recv_translate=new DirectedBuffer(DirectedBuffer::GET);
   recv_translate->SetTranslation(charset,true);
}
void Torrent::TranslateString(BeNode *node) const
{
   if(node->str_lc)
      return;
   recv_translate->ResetTranslation();
   recv_translate->PutTranslated(node->str);
   node->str_lc.nset(recv_translate->Get(),recv_translate->Size());
   recv_translate->Skip(recv_translate->Size());
}

void Torrent::SHA1(const xstring& str,xstring& buf)
{
   buf.get_space(SHA1_DIGEST_SIZE);
   sha1_buffer(str.get(),str.length(),buf.get_non_const());
   buf.set_length(SHA1_DIGEST_SIZE);
}

void Torrent::ValidatePiece(unsigned p)
{
   const xstring& buf=Torrent::RetrieveBlock(p,0,PieceLength(p));
   bool valid=false;
   if(buf.length()==PieceLength(p)) {
      xstring& sha1=xstring::get_tmp();
      SHA1(buf,sha1);
      valid=!memcmp(pieces->get()+p*SHA1_DIGEST_SIZE,sha1,SHA1_DIGEST_SIZE);
   }
   if(!valid) {
      if(buf.length()==PieceLength(p))
	 LogError(11,"piece %u digest mismatch",p);
      if(my_bitfield->get_bit(p)) {
	 total_left+=PieceLength(p);
	 complete_pieces--;
	 my_bitfield->set_bit(p,0);
      }
      piece_info[p]->block_map.clear();
   } else {
      LogNote(11,"piece %u ok",p);
      if(!my_bitfield->get_bit(p)) {
	 total_left-=PieceLength(p);
	 complete_pieces++;
	 my_bitfield->set_bit(p,1);
      }
   }
}

bool TorrentPiece::has_a_downloader() const
{
   for(int i=0; i<downloader.count(); i++)
      if(downloader[i])
	 return true;
   return false;
}

template<typename T>
static inline int cmp(T a,T b)
{
   if(a>b)
      return 1;
   if(a<b)
      return -1;
   return 0;
}

static Torrent *cmp_torrent;
int Torrent::PiecesNeededCmp(const unsigned *a,const unsigned *b)
{
   int ra=cmp_torrent->piece_info[*a]->sources_count;
   int rb=cmp_torrent->piece_info[*b]->sources_count;
   int c=cmp(ra,rb);
   if(c) return c;
   return cmp(*a,*b);
}
int Torrent::PeersCompareActivity(const SMTaskRef<TorrentPeer> *p1,const SMTaskRef<TorrentPeer> *p2)
{
   TimeDiff i1((*p1)->activity_timer.TimePassed());
   TimeDiff i2((*p2)->activity_timer.TimePassed());
   return cmp(i1.Seconds(),i2.Seconds());
}
int Torrent::PeersCompareRecvRate(const SMTaskRef<TorrentPeer> *p1,const SMTaskRef<TorrentPeer> *p2)
{
   float r1((*p1)->peer_recv_rate.Get());
   float r2((*p2)->peer_recv_rate.Get());
   int c=cmp(r1,r2);
   if(c) return c;
   return PeersCompareSendRate(p1,p2);
}
int Torrent::PeersCompareSendRate(const SMTaskRef<TorrentPeer> *p1,const SMTaskRef<TorrentPeer> *p2)
{
   float r1((*p1)->peer_send_rate.Get());
   float r2((*p2)->peer_send_rate.Get());
   return cmp(r1,r2);
}

bool Torrent::SeededEnough() const
{
   return (stop_on_ratio>0 && GetRatio()>=stop_on_ratio)
      || seed_timer.Stopped();
}

int Torrent::Do()
{
   int m=STALL;
   if(Done())
      return m;

   // retrieve metainfo if don't have already.
   if(!metainfo_tree) {
      if(!metainfo_fa) {
	 LogNote(9,"Retrieving meta-data from `%s'...\n",metainfo_url.get());
	 ParsedURL u(metainfo_url,true);
	 if(!u.proto)
	    u.proto.set("file");
	 metainfo_fa=FileAccess::New(&u);
	 metainfo_fa->Open(u.path,FA::RETRIEVE);
	 metainfo_fa->SetFileURL(metainfo_url);
	 metainfo_data=new IOBufferFileAccess(metainfo_fa);
	 m=MOVED;
      }
      if(metainfo_data->Error()) {
	 SetError(new Error(-1,metainfo_data->ErrorText(),metainfo_data->ErrorFatal()));
	 metainfo_fa->Close();
	 metainfo_data=0;
	 return MOVED;
      }
      if(!metainfo_data->Eof())
      	 return m;
      metainfo_fa->Close();
      metainfo_fa=0;
      LogNote(9,"meta-data EOF\n");
      int rest;
      metainfo_tree=BeNode::Parse(metainfo_data->Get(),metainfo_data->Size(),&rest);
      metainfo_data=0;
      if(!metainfo_tree) {
	 SetError("Meta-data parse error");
	 return MOVED;
      }
      if(rest>0) {
	 SetError("Junk at the end of Meta-data");
	 return MOVED;
      }

      InitTranslation();

      LogNote(10,"Received meta-data:");
      Log::global->Write(10,metainfo_tree->Format());

      if(metainfo_tree->type!=BeNode::BE_DICT) {
	 SetError("Meta-data: wrong top level type, must be DICT");
         return MOVED;
      }
      BeNode *announce=Lookup(metainfo_tree,"announce",BeNode::BE_STR);
      if(!announce)
         return MOVED;

      tracker_url.set(announce->str);
      LogNote(4,"Tracker URL is `%s'",tracker_url.get());
      ParsedURL u(tracker_url.get(),true);
      if(u.proto.ne("http") && u.proto.ne("https")) {
	 SetError("Meta-data: wrong `announce' protocol, must be http or https");
         return MOVED;
      }
      // fix the URL: append either ? or & if missing.
      if(tracker_url.last_char()!='?' && tracker_url.last_char()!='&')
	 tracker_url.append(strchr(tracker_url.get(),'?')?'&':'?');

      info=Lookup(metainfo_tree,"info",BeNode::BE_DICT);
      if(!info)
         return MOVED;

      SHA1(info->str,info_hash);

      BeNode *b_piece_length=Lookup(info,"piece length",BeNode::BE_INT);
      if(!b_piece_length)
	 return MOVED;
      piece_length=b_piece_length->num;
      LogNote(4,"Piece length is %u",piece_length);

      BeNode *b_name=Lookup(info,"name",BeNode::BE_STR);
      if(!b_name)
	 return MOVED;
      TranslateString(b_name);
      name=&b_name->str_lc;
      Reconfig(0);

      BeNode *files=info->dict.lookup("files");
      if(!files) {
	 single_file=true;
	 BeNode *length=Lookup(info,"length",BeNode::BE_INT);
	 if(!length)
	    return MOVED;
	 total_length=length->num;
      } else {
	 single_file=false;
	 if(files->type!=BeNode::BE_LIST) {
	    SetError("Meta-data: wrong `info/files' type, must be LIST");
	    return MOVED;
	 }
	 total_length=0;
	 for(int i=0; i<files->list.length(); i++) {
	    if(files->list[i]->type!=BeNode::BE_DICT) {
	       SetError(xstring::format("Meta-data: wrong `info/files[%d]' type, must be LIST",i));
	       return MOVED;
	    }
	    BeNode *f=Lookup(files->list[i]->dict,"length",BeNode::BE_INT);
	    if(!f)
	       return MOVED;
	    if(!Lookup(files->list[i]->dict,"path",BeNode::BE_LIST))
	       return MOVED;
	    total_length+=f->num;
	 }
      }
      LogNote(4,"Total length is %llu",total_length);
      total_left=total_length;

      last_piece_length=total_length%piece_length;
      if(last_piece_length==0)
	 last_piece_length=piece_length;

      total_pieces=(total_length+piece_length-1)/piece_length;

      BeNode *b_pieces=Lookup(info,"pieces",BeNode::BE_STR);
      if(!b_pieces)
	 return MOVED;
      pieces=&b_pieces->str;
      if(pieces->length()!=SHA1_DIGEST_SIZE*total_pieces) {
	 SetError("Meta-data: invalid `pieces' length");
	 return MOVED;
      }

      my_bitfield=new BitField(total_pieces);
      for(unsigned p=0; p<total_pieces; p++)
	 piece_info.append(new TorrentPiece(BlocksInPiece(p)));

      if(!force_valid) {
	 validate_index=0;
	 validating=true;
      } else {
	 for(unsigned i=0; i<total_pieces; i++)
	    my_bitfield->set_bit(i,1);
	 complete_pieces=total_pieces;
	 total_left=0;
	 complete=true;
	 seed_timer.Reset();
      }
   }
   if(validating) {
      ValidatePiece(validate_index++);
      if(validate_index<total_pieces)
	 return MOVED;
      fd_cache->CloseAll();
      validating=false;
      if(total_left==0) {
	 complete=true;
	 seed_timer.Reset();
      }
   }
   if(!t_session && !started && !shutting_down) {
      if(listener->GetPort()==0)
	 return m;
      ParsedURL u(tracker_url.get(),true);
      t_session=FileAccess::New(&u);
      listener->AddTorrent(this);
      SendTrackerRequest("started");
      m=MOVED;
   }

   if(peers_scan_timer.Stopped())
      ScanPeers();
   if(optimistic_unchoke_timer.Stopped())
      OptimisticUnchoke();

   // count peers
   active_peers_count=0;
   complete_peers_count=0;
   for(int i=0; i<peers.count(); i++) {
      active_peers_count+=peers[i]->Active();
      complete_peers_count+=peers[i]->Complete();
   }

   // rebuild lists of needed pieces
   if(!complete && pieces_needed_rebuild_timer.Stopped()) {
      pieces_needed.truncate();
      bool enter_end_game=true;
      for(unsigned i=0; i<total_pieces; i++) {
	 if(!my_bitfield->get_bit(i)) {
	    if(!piece_info[i]->has_a_downloader())
	       enter_end_game=false;
	    if(piece_info[i]->sources_count==0)
	       continue;
	    pieces_needed.append(i);
	 }
      }
      if(!end_game && enter_end_game) {
	 LogNote(1,"entering End Game mode");
	 end_game=true;
      }
      cmp_torrent=this;
      pieces_needed.qsort(PiecesNeededCmp);
      pieces_needed_rebuild_timer.Reset();
   }

   if(tracker_reply) {
      if(tracker_reply->Error()) {
	 SetError(new Error(-1,tracker_reply->ErrorText(),false));
	 t_session->Close();
	 tracker_reply=0;
	 tracker_timer.Reset();
	 return MOVED;
      }
      if(tracker_reply->Eof()) {
	 t_session->Close();
	 int rest;
	 Ref<BeNode> reply(BeNode::Parse(tracker_reply->Get(),tracker_reply->Size(),&rest));
	 if(!reply) {
	    LogError(3,"Tracker reply parse error (data: %s)",tracker_reply->Dump());
	    tracker_reply=0;
	    tracker_timer.Reset();
	    return MOVED;
	 }
	 LogNote(10,"Received tracker reply:");
	 Log::global->Write(10,reply->Format());

	 if(shutting_down) {
	    tracker_reply=0;
	    return MOVED;
	 }
	 started=true;

	 if(reply->type!=BeNode::BE_DICT) {
	    SetError("Reply: wrong reply type, must be DICT");
	    return MOVED;
	 }

	 BeNode *b_failure_reason=reply->dict.lookup("failure reason");
	 if(b_failure_reason) {
	    if(b_failure_reason->type==BeNode::BE_STR)
	       SetError(b_failure_reason->str);
	    else
	       SetError("Reply: wrong `failure reason' type, must be STR");
	    return MOVED;
	 }

	 BeNode *b_interval=reply->dict.lookup("interval");
	 if(b_interval && b_interval->type==BeNode::BE_INT) {
	    LogNote(4,"Tracker interval is %llu",b_interval->num);
	    tracker_timer.Set(b_interval->num);
	 }

	 BeNode *b_tracker_id=reply->dict.lookup("tracker id");
	 if(!tracker_id && b_tracker_id && b_tracker_id->type==BeNode::BE_STR)
	    tracker_id.set(b_tracker_id->str);

	 BeNode *b_peers=reply->dict.lookup("peers");
	 if(b_peers) {
	    if(b_peers->type==BeNode::BE_STR) { // binary model
	       const char *data=b_peers->str;
	       int len=b_peers->str.length();
	       while(len>=6) {
		  sockaddr_u a;
		  a.sa.sa_family=AF_INET;
		  memcpy(&a.in.sin_addr,data,4);
		  memcpy(&a.in.sin_port,data+4,2);
		  data+=6;
		  len-=6;
		  AddPeer(new TorrentPeer(this,&a));
	       }
	    } else if(b_peers->type==BeNode::BE_LIST) { // dictionary model
	       for(int p=0; p<b_peers->list.count(); p++) {
		  BeNode *b_peer=b_peers->list[p];
		  if(b_peer->type!=BeNode::BE_DICT)
		     continue;
		  BeNode *b_ip=b_peer->dict.lookup("ip");
		  if(b_ip->type!=BeNode::BE_STR)
		     continue;
		  BeNode *b_port=b_peer->dict.lookup("port");
		  if(b_port->type!=BeNode::BE_INT)
		     continue;
		  sockaddr_u a;
		  a.sa.sa_family=AF_INET;
		  if(!inet_aton(b_ip->str,&a.in.sin_addr))
		     continue;
		  a.in.sin_port=htons(b_port->num);
		  AddPeer(new TorrentPeer(this,&a));
	       }
	    }
	 }

	 tracker_timer.Reset();
	 tracker_reply=0;
      }
   } else {
      if(complete && SeededEnough()) {
	 Shutdown();
	 return MOVED;
      }
      if(tracker_timer.Stopped()) {
	 // remove uninteresting peers and request more
	 for(int i=0; i<peers.count(); i++) {
	    const TorrentPeer *peer=peers[i];
	    if(peer->ActivityTimedOut()) {
	       LogNote(4,"removing uninteresting peer %s (%s)",peer->GetName(),peers[i]->Status());
	       peers.remove(i--);
	    }
	 }
	 SendTrackerRequest(0);
      }
   }
   return m;
}

void Torrent::Accept(int s,const sockaddr_u *addr,IOBuffer *rb)
{
   if(!decline_timer.Stopped()) {
      LogNote(4,"declining new connection");
      Delete(rb);
      close(s);
      return;
   }
   TorrentPeer *p=new TorrentPeer(this,addr);
   p->Connect(s,rb);
   AddPeer(p);
}

void Torrent::AddPeer(TorrentPeer *peer)
{
   for(int i=0; i<peers.count(); i++) {
      if(peers[i]->AddressEq(peer)) {
	 if(peer->Connected() && !peers[i]->Connected()) {
	    peers[i]=peer;
	 } else {
	    delete peer;
	 }
	 return;
      }
   }
   peers.append(peer);
}

void Torrent::SendTrackerRequest(const char *event)
{
   if(!t_session)
      return;

   int numwant=complete?seed_min_peers:max_peers/2;
   if(numwant>peers.count())
      numwant-=peers.count();
   else
      numwant=0;
   if(shutting_down)
      numwant=-1;

   xstring request;
   request.setf("info_hash=%s",url::encode(info_hash,URL_PATH_UNSAFE).get());
   request.appendf("&peer_id=%s",url::encode(my_peer_id,URL_PATH_UNSAFE).get());
   request.appendf("&port=%d",listener->GetPort());
   request.appendf("&uploaded=%llu",total_sent);
   request.appendf("&downloaded=%llu",total_recv);
   request.appendf("&left=%llu",total_left);
   request.append("&compact=1&no_peer_id=1");
   if(event)
      request.appendf("&event=%s",event);
   const char *ip=ResMgr::Query("torrent:ip",0);
   if(ip && ip[0])
      request.appendf("&ip=%s",ip);
   if(numwant>=0)
      request.appendf("&numwant=%d",numwant);
   if(my_key)
      request.appendf("&key=%s",my_key.get());
   if(tracker_id)
      request.appendf("&trackerid=%s",url::encode(tracker_id,URL_PATH_UNSAFE).get());
   LogSend(4,request);
   t_session->Open(request,FA::RETRIEVE);
   t_session->SetFileURL(xstring::cat(tracker_url.get(),request.get(),NULL));
   tracker_reply=new IOBufferFileAccess(t_session);
}

const char *Torrent::MakePath(BeNode *p) const
{
   BeNode *path=p->dict.lookup("path");
   static xstring buf;
   buf.set(*name);
   if(buf.eq("..") || buf[0]=='/') {
      buf.set_substr(0,0,"_",1);
   }
   for(int i=0; i<path->list.count(); i++) {
      BeNode *e=path->list[i];
      if(e->type==BeNode::BE_STR) {
	 TranslateString(e);
	 buf.append('/');
	 if(e->str_lc.eq(".."))
	    buf.append('_');
	 buf.append(e->str_lc);
      }
   }
   return buf;
}
const char *Torrent::FindFileByPosition(unsigned piece,unsigned begin,off_t *f_pos,off_t *f_tail) const
{
   const BeNode *files=info->dict.lookup("files");
   off_t target_pos=(off_t)piece*piece_length+begin;
   if(!files) {
      *f_pos=target_pos;
      *f_tail=total_length-target_pos;
      return name->get();
   } else {
      off_t scan_pos=0;
      for(int i=0; i<files->list.length(); i++) {
	 const BeNode *f=files->list[i]->dict.lookup("length");
	 off_t file_length=f->num;
	 if(scan_pos<=target_pos && scan_pos+file_length>target_pos) {
	    *f_pos=target_pos-scan_pos;
	    *f_tail=file_length-*f_pos;
	    return MakePath(files->list[i]);
	 }
	 scan_pos+=file_length;
      }
   }
   return 0;
}

FDCache::FDCache()
   : clean_timer(1)
{
   max_count=16;
   max_time=30;
}
FDCache::~FDCache()
{
   CloseAll();
}
void FDCache::Clean()
{
   for(int i=0; i<3; i++) {
      for(const FD *f=&cache[i].each_begin(); f->last_used; f=&cache[i].each_next()) {
	 if(f->fd==-1 && f->last_used+1<now.UnixTime()) {
	    cache[i].remove(*cache[i].each_key());
	    continue;
	 }
	 if(f->last_used+max_time<now.UnixTime()) {
	    close(f->fd);
	    cache[i].remove(*cache[i].each_key());
	 }
      }
   }
   if(Count()>0)
      clean_timer.Reset();
}
int FDCache::Do()
{
   if(clean_timer.Stopped())
      Clean();
   return STALL;
}
void FDCache::Close(const char *name)
{
   const xstring n(name);
   for(int i=0; i<3; i++) {
      const FD& f=cache[i].lookup(n);
      if(f.last_used!=0) {
	 if(f.fd!=-1)
	    close(f.fd);
	 cache[i].remove(n);
      }
   }
}
void FDCache::CloseAll()
{
   for(int i=0; i<3; i++) {
      for(const FD *f=&cache[i].each_begin(); f->last_used; f=&cache[i].each_next()) {
	 if(f->fd!=-1)
	    close(f->fd);
	 cache[i].remove(*cache[i].each_key());
      }
   }
}
bool FDCache::CloseOne()
{
   int oldest_mode=0;
   int oldest_fd=-1;
   int oldest_time=0;
   const xstring *oldest_key=0;
   for(int i=0; i<3; i++) {
      for(const FD *f=&cache[i].each_begin(); f; f=&cache[i].each_next()) {
	 if(oldest_key==0 || f->last_used<oldest_time) {
	    oldest_key=cache[i].each_key();
	    oldest_time=f->last_used;
	    oldest_fd=f->fd;
	    oldest_mode=i;
	 }
      }
   }
   if(!oldest_key)
      return false;
   if(oldest_fd!=-1)
      close(oldest_fd);
   cache[oldest_mode].remove(*oldest_key);
   return true;
}
int FDCache::Count() const
{
   return cache[0].count()+cache[1].count()+cache[2].count();
}
int FDCache::OpenFile(const char *file,int m)
{
   int ci=m&3;
   assert(ci<3);
   FD& f=cache[ci].lookup_Lv(file);
   if(f.last_used!=0) {
      if(f.fd!=-1)
	 f.last_used=now.UnixTime();
      else
	 errno=f.saved_errno;
      return f.fd;
   }
   if(ci==O_RDONLY) {
      // O_RDWR also will do, check if we have it.
      const FD& f_rw=cache[O_RDWR].lookup(file);
      if(f_rw.last_used!=0 && f_rw.fd!=-1) {
	 // don't update last_used to expire it and reopen with proper mode
	 return f_rw.fd;
      }
   }
   Clean();
   clean_timer.Reset();
   ProtoLog::LogNote(9,"opening %s",file);
   int fd;
   do {
      fd=open(file,m,0664);
   } while(fd==-1 && (errno==EMFILE || errno==ENFILE) && CloseOne());
   FD new_entry = {fd,errno,now.UnixTime()};
   cache[ci].add(file,new_entry);
   return fd;
}

int Torrent::OpenFile(const char *file,int m)
{
   bool did_mkdir=false;
try_again:
   const char *cf=dir_file(output_dir,file);
   int fd=fd_cache->OpenFile(cf,m);
   while(fd==-1 && (errno==EMFILE || errno==ENFILE) && peers.count()>0) {
      peers.chop();  // free an fd
      fd=fd_cache->OpenFile(cf,m);
   }
   if(validating)
      return fd;
   if(fd==-1)
      fd_cache->Close(cf); // remove negative cache.
   if(fd==-1 && errno==ENOENT && !did_mkdir) {
      LogError(10,"open(%s): %s",cf,strerror(errno));
      const char *sl=strchr(file,'/');
      while(sl)
      {
	 if(sl>file) {
	    if(mkdir(cf=dir_file(output_dir,xstring::get_tmp(file,sl-file)),0775)==-1 && errno!=EEXIST)
	       LogError(9,"mkdir(%s): %s",cf,strerror(errno));
	 }
	 sl=strchr(sl+1,'/');
      }
      did_mkdir=true;
      goto try_again;
   }
   return fd;
}

void Torrent::SetPieceNotWanted(unsigned piece)
{
   for(int j=0; j<pieces_needed.count(); j++) {
      if(pieces_needed[j]==piece) {
	 pieces_needed.remove(j);
	 break;
      }
   }
}

void Torrent::StoreBlock(unsigned piece,unsigned begin,unsigned len,const char *buf)
{
   for(int i=0; i<peers.count(); i++)
      peers[i]->CancelBlock(piece,begin);

   unsigned b=begin/BLOCK_SIZE;
   int bc=(len+BLOCK_SIZE-1)/BLOCK_SIZE;

   off_t f_pos=0;
   off_t f_rest=len;
   while(len>0) {
      const char *file=FindFileByPosition(piece,begin,&f_pos,&f_rest);
      if(f_rest>len)
	 f_rest=len;
      int fd=OpenFile(file,O_RDWR|O_CREAT);
      if(fd==-1) {
	 SetError(xstring::format("open(%s): %s",file,strerror(errno)));
	 return;
      }
      int w=pwrite(fd,buf,f_rest,f_pos);
      int saved_errno=errno;
      if(w==-1) {
	 SetError(xstring::format("pwrite(%s): %s",file,strerror(saved_errno)));
	 return;
      }
      if(w==0) {
	 SetError(xstring::format("pwrite(%s): write error - disk full?",file));
	 return;
      }
      buf+=w;
      begin+=w;
      len-=w;
   }

   while(bc-->0) {
      piece_info[piece]->block_map.set_bit(b++,1);
   }
   if(piece_info[piece]->block_map.has_all_set() && !my_bitfield->get_bit(piece)) {
      ValidatePiece(piece);
      if(!my_bitfield->get_bit(piece)) {
	 LogError(0,"new piece %u digest mismatch",piece);
	 return;
      }
      LogNote(3,"piece %u complete",piece);
      SetPieceNotWanted(piece);
      for(int i=0; i<peers.count(); i++)
	 peers[i]->Have(piece);
      if(my_bitfield->has_all_set() && !complete) {
	 complete=true;
	 seed_timer.Reset();
	 end_game=false;
	 ScanPeers();
	 SendTrackerRequest("completed");
      }
   }
}

const xstring& Torrent::RetrieveBlock(unsigned piece,unsigned begin,unsigned len)
{
   static xstring buf;
   buf.set("");
   off_t f_pos=0;
   off_t f_rest=len;
   while(len>0) {
      const char *file=FindFileByPosition(piece,begin,&f_pos,&f_rest);
      if(f_rest>len)
	 f_rest=len;
      int fd=OpenFile(file,O_RDONLY);
      if(fd==-1)
	 return xstring::null;
      int w=pread(fd,buf.add_space(f_rest),f_rest,f_pos);
      int saved_errno=errno;
      if(w==-1) {
	 SetError(xstring::format("pread(%s): %s",file,strerror(saved_errno)));
	 return xstring::null;
      }
      if(w==0) {
// 	 buf.append_padding(len,'\0');
	 break;
      }
      buf.add_commit(w);
      begin+=w;
      len-=w;
   }
   return buf;
}

void Torrent::ScanPeers() {
   // scan existing peers
   for(int i=0; i<peers.count(); i++) {
      const TorrentPeer *peer=peers[i];
      if(peer->Failed())
	 LogError(2,"peer %s failed: %s",peer->GetName(),peer->ErrorText());
      else if(peer->Disconnected())
	 LogNote(4,"peer %s disconnected",peer->GetName());
      else if(peer->myself)
	 LogNote(4,"removing myself-connected peer %s",peer->GetName());
      else if(complete && peer->Complete())
	 LogNote(4,"removing unneeded peer %s (%s)",peer->GetName(),peers[i]->Status());
      else
	 continue;
      peers.remove(i--);
   }
   ReducePeers();
   peers_scan_timer.Reset();
}

void Torrent::ReducePeers()
{
   if(max_peers>0 && peers.count()>max_peers) {
      // remove least interesting peers.
      peers.qsort(PeersCompareActivity);
      int to_remove=peers.count()-max_peers;
      while(to_remove-->0) {
	 TimeInterval max_idle(peers.last()->activity_timer.TimePassed());
	 LogNote(3,"removing peer %s (too many; idle:%s)",peers.last()->GetName(),
	    max_idle.toString(TimeInterval::TO_STR_TERSE+TimeInterval::TO_STR_TRANSLATE));
	 peers.chop();
	 if(max_idle<60)
	    decline_timer.Set(60-max_idle);
      }
   }
   peers.qsort(complete ? PeersCompareSendRate : PeersCompareRecvRate);
   ReduceUploaders();
   ReduceDownloaders();
   UnchokeBestUploaders();
}
void Torrent::ReduceUploaders()
{
   bool rate_low = RateLow(RateLimit::GET);
   if(am_interested_peers_count < (rate_low?max_uploaders:min_uploaders+1))
      return;
   // make the slowest uninterested
   for(int i=0; i<peers.count(); i++) {
      TorrentPeer *peer=peers[i].get_non_const();
      if(peer->am_interested) {
	 if(peer->interest_timer.TimePassed() <= 30)
	    break;
	 peer->SetAmInterested(false);
	 if(am_interested_peers_count < max_uploaders)
	    break;
      }
   }
}
void Torrent::ReduceDownloaders()
{
   bool rate_low = RateLow(RateLimit::PUT);
   if(am_not_choking_peers_count < (rate_low?max_downloaders:min_downloaders+1))
      return;
   // choke the slowest
   for(int i=0; i<peers.count(); i++) {
      TorrentPeer *peer=peers[i].get_non_const();
      if(!peer->am_choking && peer->peer_interested) {
	 if(peer->choke_timer.TimePassed() <= 30)
	    break;
	 peer->SetAmChoking(true);
	 if(am_not_choking_peers_count < max_downloaders)
	    break;
      }
   }
}

bool Torrent::NeedMoreUploaders()
{
   return RateLow(RateLimit::GET) && am_interested_peers_count < max_uploaders
      && am_interested_timer.Stopped();
}
bool Torrent::AllowMoreDownloaders()
{
   return RateLow(RateLimit::PUT) && am_not_choking_peers_count < max_downloaders;
}

void Torrent::UnchokeBestUploaders()
{
   // unchoke 4 best uploaders
   int limit = 4;

   int count=0;
   for(int i=peers.count()-1; i>=0 && count<limit; i--) {
      TorrentPeer *peer=peers[i].get_non_const();
      if(!peer->Connected())
	 continue;
      if(!peer->choke_timer.Stopped())
	 continue;   // cannot change choke status yet
      if(!peer->peer_interested)
	 continue;
      peer->SetAmChoking(false);
      count++;
   }
}
void Torrent::OptimisticUnchoke()
{
   xarray<TorrentPeer*> choked_peers;
   for(int i=peers.count()-1; i>=0; i--) {
      TorrentPeer *peer=peers[i].get_non_const();
      if(!peer->Connected())
	 continue;
      if(!peer->choke_timer.Stopped())
	 continue;   // cannot change choke status yet
      if(peer->am_choking) {
	 if(!peer->peer_interested) {
	    peer->SetAmChoking(false);
	    continue;
	 }
	 choked_peers.append(peer);
	 if(peer->retry_timer.TimePassed()<60) {
	    // newly connected is more likely to be unchoked
	    choked_peers.append(peer);
	    choked_peers.append(peer);
	 }
      }
   }
   if(choked_peers.count()==0)
      return;
   choked_peers[rand()/13%choked_peers.count()]->SetAmChoking(false);
   optimistic_unchoke_timer.Reset();
}

int Torrent::PeerBytesAllowed(const TorrentPeer *peer,RateLimit::dir_t dir)
{
   float peer_rate=(dir==RateLimit::GET ? peer->peer_send_rate : peer->peer_recv_rate).Get();
   float rate=(dir==RateLimit::GET ? send_rate : recv_rate).Get();
   int min_rate = 1000;
   // the more is the opposite rate the more rate allowed, with a minimum
   int bytes = rate_limit.BytesAllowed(dir);
   bytes *= (peer_rate + min_rate);
   bytes /= (rate + active_peers_count*min_rate);
   return bytes;
}
void Torrent::PeerBytesUsed(int b,RateLimit::dir_t dir)
{
   rate_limit.BytesUsed(b,dir);
}
void Torrent::Reconfig(const char *name)
{
   const char *c=GetName();
   max_peers=ResMgr::Query("torrent:max-peers",c);
   seed_min_peers=ResMgr::Query("torrent:seed-min-peers",c);
   stop_on_ratio=ResMgr::Query("torrent:stop-on-ratio",c);
   rate_limit.Reconfig(name,metainfo_url);
}

const char *Torrent::Status()
{
   if(metainfo_data)
      return xstring::format("Getting meta-data: %s",metainfo_data->Status());
   if(validating) {
      return xstring::format("Validation: %u/%u (%u%%)",validate_index,total_pieces,
	 validate_index*100/total_pieces);
   }
   if(total_length==0)
      return "";
   xstring& buf=xstring::format("dn:%llu %sup:%llu %scomplete:%u/%u (%u%%)",
      total_recv,recv_rate.GetStrS(),total_sent,send_rate.GetStrS(),
      complete_pieces,total_pieces,
      unsigned((total_length-total_left)*100/total_length));
   if(end_game)
      buf.append(" end-game");
   buf.append(' ');
   buf.append(recv_rate.GetETAStrFromSize(total_left));
   return buf;
}


TorrentPeer::TorrentPeer(Torrent *p,const sockaddr_u *a)
   : timeout_timer(360), retry_timer(30), keepalive_timer(120),
     choke_timer(10), interest_timer(10), activity_timer(300)
{
   parent=p;
   addr=*a;
   sock=-1;
   connected=false;
   passive=false;
   myself=false;
   peer_choking=true;
   am_choking=true;
   peer_interested=false;
   am_interested=false;
   peer_complete_pieces=0;
   retry_timer.Stop();
   choke_timer.Stop();
   interest_timer.Stop();
   last_piece=NO_PIECE;
   if(addr.is_reserved() || addr.is_multicast() || addr.port()==0)
      SetError("invalid peer address");
   peer_bytes_pool[0]=peer_bytes_pool[1]=0;
   peer_recv=peer_sent=0;
}
TorrentPeer::~TorrentPeer()
{
}
void TorrentPeer::PrepareToDie()
{
   Disconnect();
}

void TorrentPeer::Connect(int s,IOBuffer *rb)
{
   sock=s;
   recv_buf=rb;
   connected=true;
   passive=true;
}

void TorrentPeer::SetError(const char *s)
{
   error=Error::Fatal(s);
   LogError(11,"fatal error: %s",s);
   Disconnect();
}

void TorrentPeer::Disconnect()
{
   Enter();
   if(Connected() && !recv_buf->Eof())
      LogNote(4,"closing connection");
   recv_queue.empty();
   ClearSentQueue();
   if(peer_bitfield) {
      for(unsigned p=0; p<parent->total_pieces; p++)
	 SetPieceHaving(p,false);
      peer_bitfield=0;
   }
   peer_id.unset();
   recv_buf=0;
   send_buf=0;
   if(sock!=-1)
      close(sock);
   sock=-1;
   connected=false;
   parent->am_interested_peers_count-=am_interested;
   am_interested=false;
   parent->am_not_choking_peers_count-=!am_choking;
   am_choking=true;
   peer_interested=false;
   peer_choking=true;
   peer_complete_pieces=0;
   retry_timer.Reset();
   choke_timer.Stop();
   interest_timer.Stop();
   // return to main pool
   parent->PeerBytesUsed(-peer_bytes_pool[RateLimit::GET],RateLimit::GET);
   parent->PeerBytesUsed(-peer_bytes_pool[RateLimit::PUT],RateLimit::PUT);
   peer_bytes_pool[0]=peer_bytes_pool[1]=0;
   Leave();
}

void TorrentPeer::SendHandshake()
{
   const char *const protocol="BitTorrent protocol";
   int proto_len=strlen(protocol);
   send_buf->PackUINT8(proto_len);
   send_buf->Put(protocol,proto_len);
   // FIXME: extensions
   send_buf->Put("\0\0\0\0\0\0\0",8);
   send_buf->Put(parent->info_hash);
   send_buf->Put(parent->my_peer_id);
   LogSend(9,"handshake");
}
TorrentPeer::unpack_status_t TorrentPeer::RecvHandshake()
{
   unsigned proto_len=0;
   if(recv_buf->Size()>0)
      proto_len=recv_buf->UnpackUINT8();

   if((unsigned)recv_buf->Size()<1+proto_len+8+SHA1_DIGEST_SIZE+Torrent::PEER_ID_LEN)
      return recv_buf->Eof() ? UNPACK_PREMATURE_EOF : UNPACK_NO_DATA_YET;

   int unpacked=1;
   const char *data=recv_buf->Get();

   xstring protocol(data+unpacked,proto_len);
   unpacked+=proto_len;
   unpacked+=8; // 8 bytes are reserved

   xstring peer_info_hash(data+unpacked,SHA1_DIGEST_SIZE);
   unpacked+=SHA1_DIGEST_SIZE;
   if(peer_info_hash.ne(parent->info_hash)) {
      LogError(0,"got info_hash: %s, wanted: %s",peer_info_hash.dump(),parent->info_hash.dump());
      SetError("peer info_hash mismatch");
      return UNPACK_WRONG_FORMAT;
   }

   peer_id.nset(recv_buf->Get()+unpacked,Torrent::PEER_ID_LEN);
   unpacked+=Torrent::PEER_ID_LEN;

   recv_buf->Skip(unpacked);
   LogRecv(4,xstring::format("handshake, %s, peer_id: %s",protocol.dump(),url::encode(peer_id,"").get()));

   return UNPACK_SUCCESS;
}

void TorrentPeer::SendDataReply()
{
   const PacketRequest *p=recv_queue.next();
   Enter(parent);
   const xstring& data=parent->RetrieveBlock(p->index,p->begin,p->req_length);
   Leave(parent);
   if(data.length()!=p->req_length) {
      if(parent->my_bitfield->get_bit(p->index))
	 parent->SetError(xstring::format("failed to read piece %u",p->index));
      return;
   }
   LogSend(6,xstring::format("piece:%u begin:%u size:%u",p->index,p->begin,p->req_length));
   PacketPiece(p->index,p->begin,data).Pack(send_buf);
   peer_sent+=data.length();
   parent->total_sent+=data.length();
   parent->send_rate.Add(data.length());
   peer_send_rate.Add(data.length());
   BytesPut(data.length());
   activity_timer.Reset();
}

int TorrentPeer::SendDataRequests(unsigned p)
{
   if(p==NO_PIECE)
      return 0;

   int sent=0;
   unsigned blocks=parent->BlocksInPiece(p);
   unsigned bytes_allowed=BytesAllowed(RateLimit::GET);
   for(unsigned b=0; b<blocks; b++) {
      if(parent->piece_info[p]->block_map.get_bit(b))
	 continue;
      if(parent->piece_info[p]->downloader[b]) {
	 if(!parent->end_game)
	    continue;
	 if(parent->piece_info[p]->downloader[b]==this)
	    continue;
	 if(FindRequest(p,b*Torrent::BLOCK_SIZE)>=0)
	    continue;
      }

      unsigned begin=b*Torrent::BLOCK_SIZE;
      unsigned len=Torrent::BLOCK_SIZE;

      if(b==blocks-1) {
	 assert(begin<parent->PieceLength(p));
	 unsigned max_len=parent->PieceLength(p)-begin;
	 if(len>max_len)
	    len=max_len;
      }

      if(bytes_allowed<len)
	 break;

      parent->SetDownloader(p,b,0,this);
      PacketRequest *req=new PacketRequest(p,b*Torrent::BLOCK_SIZE,len);
      LogSend(6,xstring::format("request piece:%u begin:%u size:%u",p,b*Torrent::BLOCK_SIZE,len));
      req->Pack(send_buf);
      sent_queue.push(req);
      SetLastPiece(p);
      sent++;
      activity_timer.Reset();
      bytes_allowed-=len;
      BytesGot(len);

      if(sent_queue.count()>=MAX_QUEUE_LEN)
	 break;
   }
   return sent;
}

void TorrentPeer::SendDataRequests()
{
   assert(am_interested);
   assert(!peer_choking);

   if(sent_queue.count()>=MAX_QUEUE_LEN)
      return;
   if(!BytesAllowedToGet(Torrent::BLOCK_SIZE))
      return;
   if(SendDataRequests(GetLastPiece())>0)
      return;

   // pick a new piece
   unsigned p=NO_PIECE;
   for(int i=0; i<parent->pieces_needed.count(); i++) {
      if(peer_bitfield->get_bit(parent->pieces_needed[i])) {
	 p=parent->pieces_needed[i];
	 if(parent->my_bitfield->get_bit(p))
	    continue;
	 // add some randomness, so that different instances don't synchronize
	 if(!parent->piece_info[p]->block_map.has_any_set()
	 && random()/13%16==0)
	    continue;
	 if(SendDataRequests(p)>0)
	    return;
      }
   }
   if(p==NO_PIECE && interest_timer.Stopped())
      SetAmInterested(false);
}

void TorrentPeer::Have(unsigned p)
{
   if(!send_buf)
      return;
   Enter();
   LogSend(9,xstring::format("have(%u)",p));
   PacketHave(p).Pack(send_buf);
   Leave();
}
int TorrentPeer::FindRequest(unsigned piece,unsigned begin) const
{
   for(int i=0; i<sent_queue.count(); i++) {
      const PacketRequest *req=sent_queue[i];
      if(req->index==piece && req->begin==begin)
	 return i;
   }
   return -1;
}
void TorrentPeer::CancelBlock(unsigned p,unsigned b)
{
   if(!send_buf)
      return;
   Enter();
   int i=FindRequest(p,b);
   if(i>=0) {
      const PacketRequest *req=sent_queue[i];
      LogSend(9,xstring::format("cancel(%u,%u)",p,b));
      PacketCancel(p,b,req->req_length).Pack(send_buf);
      parent->SetDownloader(p,b/Torrent::BLOCK_SIZE,this,0);
      sent_queue.remove(i);
   }
   Leave();
}

void TorrentPeer::ClearSentQueue(int i)
{
   while(i-->=0) {
      const PacketRequest *req=sent_queue.next();
      parent->PeerBytesGot(-req->req_length);
      parent->SetDownloader(req->index,req->begin/Torrent::BLOCK_SIZE,this,0);
   }
}

int TorrentPeer::BytesAllowed(RateLimit::dir_t dir)
{
   int a=parent->PeerBytesAllowed(this,dir);
   int pool_target=Torrent::BLOCK_SIZE*2;
   if(peer_bytes_pool[dir]<pool_target) {
      int to_pool=pool_target-peer_bytes_pool[dir];
      if(to_pool>a)
	 to_pool=a;
      peer_bytes_pool[dir]+=to_pool;
      a-=to_pool;
      parent->PeerBytesUsed(to_pool,dir);
   }
   return peer_bytes_pool[dir]+a;
}
bool TorrentPeer::BytesAllowed(RateLimit::dir_t dir,unsigned bytes)
{
   int a=BytesAllowed(dir);
   if(bytes<=(unsigned)a)
      return true;
   TimeoutS(1);
   return false;
}
void TorrentPeer::BytesUsed(int b,RateLimit::dir_t dir)
{
   if(peer_bytes_pool[dir]>=b)
      peer_bytes_pool[dir]-=b;
   else {
      b-=peer_bytes_pool[dir];
      peer_bytes_pool[dir]=0;
      parent->PeerBytesUsed(b,dir);
   }
}

unsigned TorrentPeer::GetLastPiece() const
{
   if(!peer_bitfield)
      return NO_PIECE;
   unsigned p=last_piece;
   // continue if have any blocks already
   if(p!=NO_PIECE && !parent->my_bitfield->get_bit(p)
   && parent->piece_info[p]->block_map.has_any_set()
   && peer_bitfield->get_bit(p))
      return p;
   p=parent->last_piece;
   if(p!=NO_PIECE && !parent->my_bitfield->get_bit(p)
   && peer_bitfield->get_bit(p))
      return p;
   p=last_piece;
   if(p!=NO_PIECE && !parent->my_bitfield->get_bit(p)
   && peer_bitfield->get_bit(p))
      return p;
   return NO_PIECE;
}

void TorrentPeer::SetLastPiece(unsigned p)
{
   if(last_piece==NO_PIECE || parent->my_bitfield->get_bit(last_piece))
      last_piece=p;
   if(parent->last_piece==NO_PIECE || parent->my_bitfield->get_bit(parent->last_piece))
      parent->last_piece=p;
}

void TorrentPeer::SetAmInterested(bool interest)
{
   if(am_interested==interest)
      return;
   Enter();
   LogSend(6,interest?"interested":"uninterested");
   Packet(interest?MSG_INTERESTED:MSG_UNINTERESTED).Pack(send_buf);
   parent->am_interested_peers_count+=(interest-am_interested);
   am_interested=interest;
   interest_timer.Reset();
   if(am_interested)
      parent->am_interested_timer.Reset();
   (void)BytesAllowed(RateLimit::GET); // draw some bytes from the common pool
   Leave();
}
void TorrentPeer::SetAmChoking(bool c)
{
   if(am_choking==c)
      return;
   Enter();
   LogSend(6,c?"choke":"unchoke");
   Packet(c?MSG_CHOKE:MSG_UNCHOKE).Pack(send_buf);
   parent->am_not_choking_peers_count-=(c-am_choking);
   am_choking=c;
   choke_timer.Reset();
   if(am_choking)
      recv_queue.empty();
   Leave();
}

void TorrentPeer::SetPieceHaving(unsigned p,bool have)
{
   int diff = (have - peer_bitfield->get_bit(p));
   if(!diff)
      return;
   parent->piece_info[p]->sources_count+=diff;
   peer_complete_pieces+=diff;
   peer_bitfield->set_bit(p,have);

   if(parent->piece_info[p]->sources_count==0)
      parent->SetPieceNotWanted(p);
   if(have && send_buf && !am_interested && !parent->my_bitfield->get_bit(p)
   && parent->NeedMoreUploaders()) {
      SetAmInterested(true);
      SetLastPiece(p);
   }
}

void TorrentPeer::HandlePacket(Packet *p)
{
   switch(p->GetPacketType())
   {
   case MSG_KEEPALIVE: {
	 LogRecv(5,"keep-alive");
	 break;
      }
   case MSG_CHOKE: {
	 LogRecv(5,"choke");
	 peer_choking=true;
	 ClearSentQueue(); // discard pending requests
	 break;
      }
   case MSG_UNCHOKE: {
	 LogRecv(5,"unchoke");
	 peer_choking=false;
	 if(am_interested)
	    SendDataRequests();
	 break;
      }
   case MSG_INTERESTED: {
	 LogRecv(5,"interested");
	 peer_interested=true;
	 break;
      }
   case MSG_UNINTERESTED: {
	 LogRecv(5,"uninterested");
	 recv_queue.empty();
	 peer_interested=false;
	 break;
      }
   case MSG_HAVE: {
	 PacketHave *pp=static_cast<PacketHave*>(p);
	 LogRecv(5,xstring::format("have(%u)",pp->piece));
	 if(pp->piece>=parent->total_pieces) {
	    SetError("invalid piece index");
	    break;
	 }
	 SetPieceHaving(pp->piece,true);
	 break;
      }
   case MSG_BITFIELD: {
	 PacketBitField *pp=static_cast<PacketBitField*>(p);
	 if(pp->bitfield->count()<(int)parent->total_pieces/8) {
	    LogError(9,"bitfield length %d, expected %u",pp->bitfield->count(),parent->total_pieces/8);
	    SetError("invalid bitfield length");
	    break;
	 }
	 if(pp->bitfield->has_any_set(parent->total_pieces,pp->bitfield->get_bit_length())) {
	    SetError("bitfield has spare bits set");
	    break;
	 }
	 for(unsigned p=0; p<parent->total_pieces; p++)
	    SetPieceHaving(p,pp->bitfield->get_bit(p));
	 LogRecv(5,xstring::format("bitfield(%u/%u)",peer_complete_pieces,parent->total_pieces));
	 break;
      }
   case MSG_PORT: {
	 PacketPort *pp=static_cast<PacketPort*>(p);
	 LogRecv(5,xstring::format("port(%u)",pp->port));
	 break;
      }
   case MSG_PIECE: {
	 PacketPiece *pp=static_cast<PacketPiece*>(p);
	 LogRecv(5,xstring::format("piece:%u begin:%u size:%u",pp->index,pp->begin,pp->data.length()));
	 if(pp->index>=parent->total_pieces) {
	    SetError("invalid piece index");
	    break;
	 }
	 if(pp->begin>=parent->PieceLength(pp->index)) {
	    SetError("invalid data offset");
	    break;
	 }
	 if(pp->begin+pp->data.length() > parent->PieceLength(pp->index)) {
	    SetError("invalid data length");
	    break;
	 }
	 for(int i=0; i<sent_queue.count(); i++) {
	    const PacketRequest *req=sent_queue[i];
	    if(req->index==pp->index && req->begin==pp->begin) {
	       ClearSentQueue(i); // including previous unanswered requests
	       parent->PeerBytesGot(pp->data.length()); // re-take the bytes returned by ClearSentQueue
	       break;
	    }
	 }
	 Enter(parent);
	 parent->StoreBlock(pp->index,pp->begin,pp->data.length(),pp->data.get());
	 Leave(parent);

	 int len=pp->data.length();
	 peer_recv+=len;
	 parent->total_recv+=len;
	 parent->recv_rate.Add(len);
	 peer_recv_rate.Add(len);

	 // request another block from the same piece
	 if(am_interested && !peer_choking)
	    SendDataRequests(pp->index);
	 break;
      }
   case MSG_REQUEST: {
	 PacketRequest *pp=static_cast<PacketRequest*>(p);
	 LogRecv(5,xstring::format("request for piece:%u begin:%u size:%u",pp->index,pp->begin,pp->req_length));
	 if(pp->req_length>Torrent::BLOCK_SIZE*2) {
	    SetError("too large request");
	    break;
	 }
	 if(am_choking)
	    break;
	 if(pp->index>=parent->total_pieces) {
	    SetError("invalid piece index");
	    break;
	 }
	 if(pp->begin>=parent->PieceLength(pp->index)) {
	    SetError("invalid data offset");
	    break;
	 }
	 if(pp->begin+pp->req_length > parent->PieceLength(pp->index)) {
	    SetError("invalid data length");
	    break;
	 }
	 if(recv_queue.count()>=MAX_QUEUE_LEN*16) {
	    SetError("too many requests");
	    break;
	 }
	 recv_queue.push(pp);
	 activity_timer.Reset();
	 p=0;
	 break;
      }
   case MSG_CANCEL: {
	 PacketCancel *pp=static_cast<PacketCancel*>(p);
	 LogRecv(5,xstring::format("cancel(%u,%u)",pp->index,pp->begin));
	 for(int i=0; i<recv_queue.count(); i++) {
	    const PacketRequest *req=recv_queue[i];
	    if(req->index==pp->index && req->begin==pp->begin) {
	       recv_queue.remove(i);
	       break;
	    }
	 }
	 break;
      }
   }
   delete p;
}

bool TorrentPeer::HasNeededPieces()
{
   if(GetLastPiece()!=NO_PIECE)
      return true;
   if(!peer_bitfield)
      return false;
   for(int i=0; i<parent->pieces_needed.count(); i++)
      if(peer_bitfield->get_bit(parent->pieces_needed[i]))
	 return true;
   return false;
}

int TorrentPeer::Do()
{
   int m=STALL;
   if(error || myself)
      return m;
   if(sock==-1) {
      if(passive)
	 return m;
      if(!retry_timer.Stopped())
	 return m;
      sock=SocketCreateTCP(addr.sa.sa_family,0);
      if(sock==-1)
      {
	 if(NonFatalError(errno))
	    return m;
	 SetError(xstring::format(_("cannot create socket of address family %d"),addr.sa.sa_family));
	 return MOVED;
      }
      LogNote(4,_("Connecting to peer %s port %u"),SocketNumericAddress(&addr),SocketPort(&addr));
      connected=false;
   }
   if(!connected) {
      int res=SocketConnect(sock,&addr);
      if(res==-1 && errno!=EINPROGRESS && errno!=EALREADY && errno!=EISCONN)
      {
	 int e=errno;
	 LogError(4,"connect: %s\n",strerror(e));
	 Disconnect();
	 if(FA::NotSerious(e))
	    return MOVED;
	 SetError(strerror(e));
	 return MOVED;
      }
      if(res==-1 && errno!=EISCONN) {
	 Block(sock,POLLOUT);
	 return m;
      }
      connected=true;
      timeout_timer.Reset();
      m=MOVED;
   }
   if(!recv_buf) {
      recv_buf=new IOBufferFDStream(new FDStream(sock,"<input-socket>"),IOBuffer::GET);
   }
   if(!send_buf) {
      send_buf=new IOBufferFDStream(new FDStream(sock,"<output-socket>"),IOBuffer::PUT);
      SendHandshake();
   }
   if(send_buf->Error())
   {
      LogError(2,"send: %s",send_buf->ErrorText());
      Disconnect();
      return MOVED;
   }
   if(recv_buf->Error())
   {
      LogError(2,"recieve: %s",recv_buf->ErrorText());
      Disconnect();
      return MOVED;
   }
   if(!peer_id) {
      // expect handshake
      unpack_status_t s=RecvHandshake();
      if(s==UNPACK_NO_DATA_YET)
	 return m;
      if(s!=UNPACK_SUCCESS) {
	 if(s==UNPACK_PREMATURE_EOF) {
	    if(recv_buf->Size()>0)
	       LogError(2,"peer unexpectedly closed connection after %s",recv_buf->Dump());
	    else
	       LogError(4,"peer closed connection");
	 }
	 Disconnect();
	 return MOVED;
      }
      timeout_timer.Reset();
      myself=peer_id.eq(Torrent::my_peer_id);
      if(myself)
	 return MOVED;
      peer_bitfield=new BitField(parent->total_pieces);
      if(parent->my_bitfield->has_any_set()) {
	 LogSend(5,"bitfield");
	 PacketBitField(parent->my_bitfield).Pack(send_buf);
      }
#if 0 // no DHT yet
	 LogSend(5,xstring::format("port(%d)",dht_port));
	 PacketPort(dht_port).Pack(send_buf);
#endif

      keepalive_timer.Reset();
   }

   if(keepalive_timer.Stopped()) {
      LogSend(5,"keep-alive");
      Packet(MSG_KEEPALIVE).Pack(send_buf);
      keepalive_timer.Reset();
   }

   if(send_buf->Size()>(int)Torrent::BLOCK_SIZE*4)
      recv_buf->Suspend();
   else
      recv_buf->Resume();

   if(recv_buf->IsSuspended())
      return m;

   timeout_timer.Reset(send_buf->EventTime());
   timeout_timer.Reset(recv_buf->EventTime());
   if(timeout_timer.Stopped()) {
      LogError(0,_("Timeout - reconnecting"));
      Disconnect();
      return MOVED;
   }

   if(!am_interested && interest_timer.Stopped()
   && HasNeededPieces() && parent->NeedMoreUploaders())
      SetAmInterested(true);

   if(am_interested && !peer_choking && sent_queue.count()<MAX_QUEUE_LEN)
      SendDataRequests();

   if(peer_interested && am_choking && choke_timer.Stopped()
   && parent->AllowMoreDownloaders())
      SetAmChoking(false);

   if(recv_queue.count()>0 && send_buf->Size()<(int)Torrent::BLOCK_SIZE*2) {
      unsigned bytes_allowed=BytesAllowed(RateLimit::PUT);
      while(bytes_allowed>=recv_queue[0]->req_length) {
	 bytes_allowed-=recv_queue[0]->req_length;
	 SendDataReply();
	 m=MOVED;
	 if(!Connected())
	    return m;
	 if(recv_queue.count()==0)
	    break;
	 if(send_buf->Size()>=(int)Torrent::BLOCK_SIZE)
	    m|=send_buf->Do();
	 if(send_buf->Size()>=(int)Torrent::BLOCK_SIZE*2)
	    break;
      }
   }

   if(recv_buf->Eof() && recv_buf->Size()==0) {
      LogError(4,"peer closed connection");
      Disconnect();
      return MOVED;
   }

   Packet *reply=0;
   unpack_status_t st=UnpackPacket(recv_buf,&reply);
   if(st==UNPACK_NO_DATA_YET)
      return m;
   if(st!=UNPACK_SUCCESS)
   {
      if(st==UNPACK_PREMATURE_EOF)
	 LogError(2,"peer unexpectedly closed connection after %s",recv_buf->Dump());
      else
	 LogError(2,"invalid peer response format");
      Disconnect();
      return MOVED;
   }
   reply->DropData(recv_buf);
   HandlePacket(reply);
   return MOVED;
}

TorrentPeer::unpack_status_t TorrentPeer::UnpackPacket(Ref<IOBuffer>& b,TorrentPeer::Packet **p)
{
   Packet *&pp=*p;
   pp=0;

   Ref<Packet> probe(new Packet);
   unpack_status_t res=probe->Unpack(b);
   if(res!=UNPACK_SUCCESS)
      return res;

   Log::global->Format(11,"<--- got a packet, length=%d, type=%d(%s)\n",
      probe->GetLength(),probe->GetPacketType(),probe->GetPacketTypeText());

   switch(probe->GetPacketType())
   {
   case MSG_KEEPALIVE:
   case MSG_CHOKE:
   case MSG_UNCHOKE:
   case MSG_INTERESTED:
   case MSG_UNINTERESTED:
      pp=probe.borrow();
      break;
   case MSG_HAVE:
      pp=new PacketHave();
      break;
   case MSG_BITFIELD:
      pp=new PacketBitField();
      break;
   case MSG_REQUEST:
      pp=new PacketRequest();
      break;
   case MSG_PIECE:
      pp=new PacketPiece();
      break;
   case MSG_CANCEL:
      pp=new PacketCancel();
      break;
   case MSG_PORT:
      pp=new PacketPort();
      break;
   }
   if(probe)
      res=pp->Unpack(b);
   if(res!=UNPACK_SUCCESS)
   {
      switch(res)
      {
      case UNPACK_PREMATURE_EOF:
	 LogError(0,"premature eof");
	 break;
      case UNPACK_WRONG_FORMAT:
	 LogError(0,"wrong packet format");
	 break;
      case UNPACK_NO_DATA_YET:
      case UNPACK_SUCCESS:
	 ;
      }
      if(probe)
	 probe->DropData(b);
      else
	 pp->DropData(b);
      delete pp;
      pp=0;
   }
   return res;
}

const char *TorrentPeer::Packet::GetPacketTypeText() const
{
   const char *const text_table[]={
      "keep-alive", "choke", "unchoke", "interested", "uninterested",
      "have", "bitfield", "request", "piece", "cancel", "port"
   };
   return text_table[type+1];
}

TorrentPeer::unpack_status_t TorrentPeer::Packet::Unpack(const Buffer *b)
{
   unpacked=0;
   if(b->Size()<4)
      return b->Eof()?UNPACK_PREMATURE_EOF:UNPACK_NO_DATA_YET;
   length=b->UnpackUINT32BE(0);
   unpacked+=4;
   if(length==0) {
      type=MSG_KEEPALIVE;
      return UNPACK_SUCCESS;
   }
   if(b->Size()<length+4)
      return b->Eof()?UNPACK_PREMATURE_EOF:UNPACK_NO_DATA_YET;
   int t=b->UnpackUINT8(4);
   unpacked++;
   if(!is_valid_reply(t))
      return UNPACK_WRONG_FORMAT;
   type=(packet_type)t;
   return UNPACK_SUCCESS;
}

bool TorrentPeer::AddressEq(const TorrentPeer *o) const
{
   return !memcmp(&addr,&o->addr,sizeof(addr));
}

const char *TorrentPeer::GetName() const
{
   return xstring::format("[%s]:%d",addr.address(),addr.port());
}

const char *TorrentPeer::Status()
{
   if(sock==-1)
      return "Not connected";
   if(!connected)
      return "Connecting...";
   if(!peer_id)
      return "Handshaking...";
   xstring &buf=xstring::format("dn:%llu %sup:%llu %s",
      peer_recv,peer_recv_rate.GetStrS(),peer_sent,peer_send_rate.GetStrS());
   if(peer_interested)
      buf.append("peer-interested ");
   if(peer_choking)
      buf.append("peer-choking ");
   if(am_interested)
      buf.append("am-interested ");
   if(am_choking)
      buf.append("am-choking ");
   buf.appendf("complete:%u/%u (%u%%)",peer_complete_pieces,parent->total_pieces,
      peer_complete_pieces*100/parent->total_pieces);
   return buf;
}

TorrentPeer::Packet::Packet(packet_type t)
{
   type=t;
   length=0;
   if(type>=0)
      length+=1;
}
void TorrentPeer::Packet::Pack(Ref<IOBuffer>& b)
{
   b->PackUINT32BE(length);
   if(type>=0)
      b->PackUINT8(type);
}

TorrentPeer::PacketBitField::PacketBitField(const BitField *bf)
   : Packet(MSG_BITFIELD)
{
   bitfield=new BitField();
   bitfield->set(*bf);
   length+=bitfield->count();
}
TorrentPeer::PacketBitField::~PacketBitField()
{
}
TorrentPeer::unpack_status_t TorrentPeer::PacketBitField::Unpack(const Buffer *b)
{
   unpack_status_t res;
   res=Packet::Unpack(b);
   if(res!=UNPACK_SUCCESS)
      return res;
   int bytes=length+4-unpacked;
   bitfield=new BitField(bytes*8);
   memcpy(bitfield->get_non_const(),b->Get()+unpacked,bytes);
   unpacked+=bytes;
   return UNPACK_SUCCESS;
}
void TorrentPeer::PacketBitField::ComputeLength()
{
   Packet::ComputeLength();
   length+=bitfield->count();
}
void TorrentPeer::PacketBitField::Pack(Ref<IOBuffer>& b)
{
   Packet::Pack(b);
   b->Put((const char*)(bitfield->get()),bitfield->count());
}

TorrentPeer::PacketRequest::PacketRequest(unsigned i,unsigned b,unsigned l)
   : Packet(MSG_REQUEST), index(i), begin(b), req_length(l)
{
   length+=12;
}
TorrentPeer::unpack_status_t TorrentPeer::PacketRequest::Unpack(const Buffer *b)
{
   unpack_status_t res;
   res=Packet::Unpack(b);
   if(res!=UNPACK_SUCCESS)
      return res;
   index=b->UnpackUINT32BE(unpacked);unpacked+=4;
   begin=b->UnpackUINT32BE(unpacked);unpacked+=4;
   req_length=b->UnpackUINT32BE(unpacked);unpacked+=4;
   return UNPACK_SUCCESS;
}
void TorrentPeer::PacketRequest::ComputeLength()
{
   Packet::ComputeLength();
   length+=12;
}
void TorrentPeer::PacketRequest::Pack(Ref<IOBuffer>& b)
{
   Packet::Pack(b);
   b->PackUINT32BE(index);
   b->PackUINT32BE(begin);
   b->PackUINT32BE(req_length);
}


BitField::BitField(int bits) {
   bit_length=bits;
   int bytes=(bits+7)/8;
   get_space(bytes);
   memset(buf,0,bytes);
   set_length(bytes);
}
bool BitField::get_bit(int i) const {
   return (*this)[i/8]&(0x80>>(i%8));
}
void BitField::set_bit(int i,bool value) {
   unsigned char &b=(*this)[i/8];
   int v=(0x80>>(i%8));
   if(value)
      b|=v;
   else
      b&=~v;
}
bool BitField::has_any_set(int from,int to) const {
   for(int i=from; i<to; i++)
      if(get_bit(i))
	 return true;
   return false;
}
bool BitField::has_all_set(int from,int to) const {
   for(int i=from; i<to; i++)
      if(!get_bit(i))
	 return false;
   return true;
}


TorrentListener::TorrentListener()
{
   sock=-1;
}
TorrentListener::~TorrentListener()
{
   if(sock!=-1)
      close(sock);
}
void TorrentListener::AddTorrent(Torrent *t)
{
   torrents.add(t->GetInfoHash(),t);
}
void TorrentListener::RemoveTorrent(Torrent *t)
{
   torrents.remove(t->GetInfoHash());
}
int TorrentListener::Do()
{
   int m=STALL;
   if(error)
      return m;
   if(sock==-1) {
      sock=SocketCreateTCP(AF_INET,0);
      // Try to assign a port from given range
      Range range(ResMgr::Query("torrent:port-range",0));
      for(int t=0; ; t++)
      {
	 if(t>=10)
	 {
	    close(sock);
	    sock=-1;
	    TimeoutS(1);	 // retry later.
	    return m;
	 }
	 if(t==9)
	    ReuseAddress(sock);   // try to reuse address.

	 int port=0;
	 if(!range.IsFull())
	    port=range.Random();

	 if(!port)
	     break;	// nothing to bind

	 addr.sa.sa_family=AF_INET;
	 addr.in.sin_port=htons(port);

	 if(addr.bind_to(sock)==0)
	    break;
	 int saved_errno=errno;

	 // Fail unless socket was already taken
	 if(errno!=EINVAL && errno!=EADDRINUSE)
	 {
	    LogError(0,"bind([%s]:%d): %s",addr.address(),port,strerror(saved_errno));
	    close(sock);
	    sock=-1;
	    if(NonFatalError(errno))
	    {
	       TimeoutS(1);
	       return m;
	    }
	    error=Error::Fatal("Cannot bind a socket for torrent:port-range");
	    return MOVED;
	 }
	 LogError(10,"bind([%s]:%d): %s",addr.address(),port,strerror(saved_errno));
      }
      listen(sock,5);

      // get the allocated port
      socklen_t addr_len=sizeof(addr);
      getsockname(sock,&addr.sa,&addr_len);
      m=MOVED;
   }

   if(rate.Get()>5)
   {
      TimeoutS(1);
      return m;
   }

   sockaddr_u remote_addr;
   int a=SocketAccept(sock,&remote_addr);
   if(a==-1) {
      Block(sock,POLLIN);
      return m;
   }
   rate.Add(1);
   LogNote(3,"Accepted connection from [%s]:%d",remote_addr.address(),remote_addr.port());
   (void)new TorrentDispatcher(a,&remote_addr);
   m=MOVED;

   return m;
}

void TorrentListener::Dispatch(const xstring& info_hash,int sock,const sockaddr_u *remote_addr,IOBuffer *recv_buf)
{
   Torrent *t=torrents.lookup(info_hash);
   if(!t) {
      LogError(1,"peer sent unknown info_hash=%s in handshake",info_hash.dump());
      close(sock);
      delete recv_buf;
      return;
   }
   t->Accept(sock,remote_addr,recv_buf);
}

TorrentDispatcher::TorrentDispatcher(int s,const sockaddr_u *a)
   : sock(s), addr(*a),
     recv_buf(new IOBufferFDStream(new FDStream(sock,"<input-socket>"),IOBuffer::GET)),
     timeout_timer(60)
{
}
TorrentDispatcher::~TorrentDispatcher()
{
   if(sock!=-1)
      close(sock);
}
int TorrentDispatcher::Do()
{
   if(timeout_timer.Stopped())
   {
      LogError(1,"peer handshake timeout");
      deleting=true;
      return MOVED;
   }

   unsigned proto_len=0;
   if(recv_buf->Size()>0)
      proto_len=recv_buf->UnpackUINT8();

   if((unsigned)recv_buf->Size()<1+proto_len+8+SHA1_DIGEST_SIZE) {
      if(recv_buf->Eof()) {
	 if(recv_buf->Size()>0)
	    LogError(1,"peer short handshake");
	 else
	    LogError(4,"peer closed connection");
	 deleting=true;
	 return MOVED;
      }
      return STALL;
   }

   int unpacked=1;
   const char *data=recv_buf->Get();

   unpacked+=proto_len;
   unpacked+=8; // 8 bytes are reserved

   xstring peer_info_hash(data+unpacked,SHA1_DIGEST_SIZE);
   unpacked+=SHA1_DIGEST_SIZE;

   const Ref<TorrentListener>& listener=Torrent::GetListener();
   if(listener) {
      listener->Dispatch(peer_info_hash,sock,&addr,recv_buf.borrow());
      sock=-1;
   }
   deleting=true;
   return MOVED;
}

///
TorrentJob::TorrentJob(Torrent *t)
   : torrent(t), completed(false), done(false)
{
}
TorrentJob::~TorrentJob()
{
}
void TorrentJob::PrepareToDie()
{
   torrent=0;
   Job::PrepareToDie();
}

int TorrentJob::Do()
{
   if(done)
      return STALL;
   if(torrent->Done()) {
      done=true;
      const Error *e=torrent->GetInvalidCause();
      if(e)
	 eprintf("%s\n",e->Text());
      return MOVED;
   }
   if(!completed && torrent->Complete()) {
      if(parent->WaitsFor(this)) {
	 PrintStatus(1,"");
	 printf("Seeding in background...\n");
	 parent->RemoveWaiting(this);
      }
      completed=true;
      return MOVED;
   }
   return STALL;
}

void TorrentJob::PrintStatus(int v,const char *tab)
{
   const char *name=torrent->GetName();
   if(name)
      printf("%sName: %s\n",tab,name);
   printf("%s%s\n",tab,torrent->Status());
   if(torrent->GetRatio()>0)
      printf("%sratio: %f\n",tab,torrent->GetRatio());
   if(v>2) {
      printf("%sinfo hash: %s\n",tab,torrent->GetInfoHash().dump());
      printf("%stotal length: %llu\n",tab,torrent->TotalLength());
      printf("%spiece length: %u\n",tab,torrent->PieceLength());
      printf("%snext tracker request in %s\n",tab,torrent->TrackerTimerTimeLeft());
   }

   if(torrent->GetPeersCount()<=5 || v>1) {
      const TaskRefArray<TorrentPeer>& peers=torrent->GetPeers();
      for(int i=0; i<peers.count(); i++)
	 printf("%s  %s: %s\n",tab,peers[i]->GetName(),peers[i]->Status());
   } else {
      printf("%s  peers:%d active:%d complete:%d\n",tab,
	 torrent->GetPeersCount(),torrent->GetActivePeersCount(),
	 torrent->GetCompletePeersCount());
   }
}

void TorrentJob::ShowRunStatus(const SMTaskRef<StatusLine>& s)
{
   s->Show("%s",torrent->Status());
}

int TorrentJob::AcceptSig(int sig)
{
   if(!torrent)
      return WANTDIE;
   torrent->Shutdown();
   return MOVED;
}


#include "CmdExec.h"

CMD(torrent)
{
#define args (parent->args)
#define eprintf parent->eprintf
   enum {
      OPT_OUTPUT_DIRECTORY,
      OPT_FORCE_VALID,
   };
   static const struct option torrent_opts[]=
   {
      {"output-directory",required_argument,0,OPT_OUTPUT_DIRECTORY},
      {"force-valid",no_argument,0,OPT_FORCE_VALID},
      {0}
   };
   const char *output_dir=0;
   bool force_valid=false;

   args->rewind();
   int opt;
   while((opt=args->getopt_long("O:",torrent_opts,0))!=EOF)
   {
      switch(opt)
      {
      case(OPT_OUTPUT_DIRECTORY):
      case('O'):
	 output_dir=optarg;
	 break;
      case(OPT_FORCE_VALID):
	 force_valid=true;
	 break;
      case('?'):
      try_help:
	 eprintf(_("Try `help %s' for more information.\n"),args->a0());
	 return 0;
      }
   }
   args->back();
   const char *torrent=args->getnext();
   if(!torrent)
   {
      eprintf(_("%s: Please specify meta-info file or URL.\n"),args->a0());
      goto try_help;
   }
   if(args->getnext()) {
      eprintf(_("%s: Too many arguments.\n"),args->a0());
      goto try_help;
   }

   xstring_ca cwd(xgetcwd());
   if(output_dir)
      output_dir=dir_file(cwd,expand_home_relative(output_dir));
   else
      output_dir=cwd;

   Torrent *t=new Torrent(torrent,cwd,output_dir);
   if(force_valid)
      t->ForceValid();

   return new TorrentJob(t);

#undef args
}

#include "modconfig.h"
#ifdef MODULE_CMD_TORRENT
void module_init()
{
   CmdExec::RegisterCommand("torrent",cmd_torrent);
}
#endif
