﻿#include "chatroom_callback.h"
#include "gui/chatroom_form.h"
#include "gui/chatroom_frontpage.h"

namespace nim_chatroom
{

void ChatroomCallback::OnReceiveMsgCallback(__int64 room_id, const ChatRoomMessage& result)
{
	QLOG_PRO(L"Chatroom:OnReceiveMsgCallback: {0}") << result.client_msg_id_;
	//QLOG_PRO(L"Chatroom:OnReceiveMsgCallback ext: {0}") << result.ext_;
	//QLOG_PRO(L"Chatroom:OnReceiveMsgCallback from_ext: {0}") << result.from_ext_;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL && result.msg_type_ != kNIMChatRoomMsgTypeUnknown)
		{
			chat_form->OnReceiveMsgCallback(result);
		}
	};
	Post2UI(cb);
}


void ChatroomCallback::OnSendMsgCallback(__int64 room_id, int error_code, const ChatRoomMessage& result)
{
	QLOG_APP(L"Chatroom:OnSendMsgCallback: id={0} msg_id={1} code={2}") << result.room_id_ << result.client_msg_id_ << error_code;

// 	StdClosure cb = [=](){
// 
// 	};
// 	Post2UI(cb);

}


void ChatroomCallback::OnEnterCallback(__int64 room_id, const NIMChatRoomEnterStep step, int error_code, const ChatRoomInfo& info, const ChatRoomMemberInfo& my_info)
{
	QLOG_APP(L"Chatroom:OnEnterCallback: id={0} step={1} code={2}") << room_id << step << error_code;

	if (step != kNIMChatRoomEnterStepRoomAuthOver)
		return;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL)
		{
			chat_form->OnEnterCallback(error_code, info, my_info);
		}
	};
	Post2UI(cb);
}

void ChatroomCallback::OnExitCallback(__int64 room_id, int error_code, NIMChatRoomExitReason exit_reason)
{
	QLOG_APP(L"Chatroom:OnExitCallback: id={0} code={1}") << room_id << error_code;

	StdClosure cb = [room_id, exit_reason]()
	{
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form)
			chat_form->Close(ChatroomForm::kAllowClose);

		if (exit_reason == kNIMChatRoomExitReasonExit)
			return;

		ChatroomFrontpage* front_page = nim_ui::WindowsManager::GetInstance()->SingletonShow<ChatroomFrontpage>(ChatroomFrontpage::kClassName);
		if (!front_page) return;

		std::wstring kick_tip_str;
		ui::MutiLanSupport *multilan = ui::MutiLanSupport::GetInstance();
		switch (exit_reason)
		{
		case kNIMChatRoomExitReasonKickByManager:
			kick_tip_str = multilan->GetStringViaID(L"STRID_CHATROOM_TIP_KICKED");
			break;
		case kNIMChatRoomExitReasonBeBlacklisted:
			kick_tip_str = multilan->GetStringViaID(L"STRID_CHATROOM_TIP_BLACKLISTED");
			break;
		case kNIMChatRoomExitReasonKickByMultiSpot:
			kick_tip_str = multilan->GetStringViaID(L"STRID_CHATROOM_TIP_MULTIPOT_LOGIN");
			break;
		default:
			QLOG_APP(L"Exit reason: %d.") << room_id << exit_reason;
			return;
		}

		ui::Box* kicked_tip_box = (ui::Box*)front_page->FindControl(L"kicked_tip_box");
		kicked_tip_box->SetVisible(true);
		nbase::ThreadManager::PostDelayedTask(front_page->ToWeakCallback([kicked_tip_box]() {
			kicked_tip_box->SetVisible(false);
		}), nbase::TimeDelta::FromSeconds(2));

		ui::Label* kick_tip_label = (ui::Label*)kicked_tip_box->FindSubControl(L"kick_tip");
		kick_tip_label->SetText(kick_tip_str);

		ui::Label* room_name_label = (ui::Label*)kicked_tip_box->FindSubControl(L"room_name");
		room_name_label->SetDataID(nbase::Int64ToString16(room_id));
		ChatRoomInfo info = front_page->GetRoomInfo(room_id);
		if (!info.name_.empty())
			room_name_label->SetUTF8Text(info.name_);
		else
			room_name_label->SetText(nbase::StringPrintf(L"直播间(id %lld)", room_id));
	};
	Post2UI(cb);
}

void ChatroomCallback::OnNotificationCallback(__int64 room_id, const ChatRoomNotification& notification)
{
	QLOG_APP(L"Chatroom:OnNotificationCallback: id={0}") << room_id;
	//QLOG_APP(L"Chatroom:OnNotificationCallback: ext : {0}") << notification.ext_;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL)
		{
			chat_form->OnNotificationCallback(notification);
		}
	};
	Post2UI(cb);
}

void ChatroomCallback::OnRegLinkConditionCallback(__int64 room_id, const NIMChatRoomLinkCondition condition)
{
	QLOG_APP(L"Chatroom:OnRegLinkConditionCallback: id={0} condition={1}") << room_id << condition;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL)
		{
			chat_form->OnRegLinkConditionCallback(room_id, condition);
		}
	};
	Post2UI(cb);

}

void ChatroomCallback::InitChatroomCallback()
{
	ChatRoom::RegReceiveMsgCb(nbase::Bind(&ChatroomCallback::OnReceiveMsgCallback, std::placeholders::_1, std::placeholders::_2));
	ChatRoom::RegSendMsgAckCb(nbase::Bind(&ChatroomCallback::OnSendMsgCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ChatRoom::RegEnterCb(nbase::Bind(&ChatroomCallback::OnEnterCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	ChatRoom::RegExitCb(nbase::Bind(&ChatroomCallback::OnExitCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ChatRoom::RegNotificationCb(nbase::Bind(&ChatroomCallback::OnNotificationCallback, std::placeholders::_1, std::placeholders::_2));
	ChatRoom::RegLinkConditionCb(nbase::Bind(&ChatroomCallback::OnRegLinkConditionCallback, std::placeholders::_1, std::placeholders::_2));
}


}