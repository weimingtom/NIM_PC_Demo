﻿#include "team_search.h"
#include "module/emoji/richedit_util.h"
#include "gui/session/control/session_util.h"
#include "callback/team/team_callback.h"

using namespace ui;

namespace nim_comp
{
const LPCTSTR TeamSearchForm::kClassName = L"TeamSearchForm";

TeamSearchForm::TeamSearchForm()
{

}

TeamSearchForm::~TeamSearchForm()
{

}

std::wstring TeamSearchForm::GetSkinFolder()
{
	return L"team_info";
}

std::wstring TeamSearchForm::GetSkinFile()
{
	return L"team_search.xml";
}

ui::UILIB_RESOURCETYPE TeamSearchForm::GetResourceType() const
{
	return ui::UILIB_FILE;
}

std::wstring TeamSearchForm::GetZIPFileName() const
{
	return L"team_search.zip";
}

std::wstring TeamSearchForm::GetWindowClassName() const
{
	return kClassName;
}

std::wstring TeamSearchForm::GetWindowId() const
{
	return kClassName;
}

UINT TeamSearchForm::GetClassStyle() const
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}

LRESULT TeamSearchForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_KEYDOWN && wParam == 'V')
	{
		if(::GetKeyState(VK_CONTROL) < 0)
		{
			if(re_tid_->IsFocused())
			{
				re_tid_->PasteSpecial(CF_TEXT);
				return 1;
			}
		}
	}
	return __super::HandleMessage(uMsg, wParam, lParam);
}

void TeamSearchForm::InitWindow()
{
	SetTaskbarTitle(L"搜索加入群");
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&TeamSearchForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&TeamSearchForm::OnClicked, this, std::placeholders::_1));

	tsp_[TSP_SEARCH] = FindControl(L"page_search");
	tsp_[TSP_APPLY] = FindControl(L"page_apply");
	tsp_[TSP_APPLY_OK] = FindControl(L"page_apply_ok");

	re_tid_ = (RichEdit*) FindControl(L"re_tid");
	re_tid_->SetLimitText(20);

	error_tip_ = (Label*) FindControl(L"error_tip");
	btn_search_ = (Button*) FindControl(L"btn_search");

	team_icon_ = (Button*) FindControl(L"team_icon");
	team_name_ = (Label*) FindControl(L"team_name");
	team_id_ = (Label*) FindControl(L"team_id");
	label_group_ = (Label*) FindControl(L"label_group");
	btn_apply_ = (Button*) FindControl(L"btn_apply");

	re_apply_ = (RichEdit*) FindControl(L"re_apply");

	GotoPage(TSP_SEARCH);
}

void TeamSearchForm::GotoPage( TeamSearchPage page )
{
	if(page == TSP_SEARCH)
	{
		tid_.clear();
		re_tid_->SetText(L"");
		btn_search_->SetEnabled(true);
	}

	for(int i = 0; i < TSP_COUNT; i++)
	{
		tsp_[i]->SetVisible(false);
	}
	tsp_[page]->SetVisible(true);
}

void TeamSearchForm::ShowTeamInfo(const std::string &info)
{
	team_icon_->SetBkImage( TeamService::GetInstance()->GetTeamPhoto(false) );

	Json::Value json;
	if( StringToJson(info, json) )
	{
		team_id_->SetUTF8Text(tid_);

		const Json::Value &tinfo = json[nim::kNIMNotificationKeyData][nim::kNIMNotificationGetTeamInfoKey];
		std::string name = tinfo[nim::kNIMTeamInfoKeyName].asString();
		if( name.empty() )
		{
			tname_ = nbase::UTF8ToUTF16(tid_);
		}
		else
		{
			tname_ = nbase::UTF8ToUTF16(name);
			team_name_->SetText(tname_);
		}

		nim::NIMTeamType type = (nim::NIMTeamType)tinfo[nim::kNIMTeamInfoKeyType].asInt();
		if (type == nim::kNIMTeamTypeNormal)
		{
			label_group_->SetVisible(true);
			btn_apply_->SetEnabled(false);
		}
		else
		{
			label_group_->SetVisible(false);
			btn_apply_->SetEnabled(true);
		}
	}
	else
	{
		btn_apply_->SetEnabled(false);
	}
}

bool TeamSearchForm::Notify(ui::EventArgs* arg)
{
	std::wstring name = arg->pSender->GetName();
	if(arg->Type == kEventTextChange)
	{
		if(name == L"re_tid")
		{
			error_tip_->SetVisible(false);
			btn_search_->SetEnabled(true);
		}
	}
	return false;
}

bool TeamSearchForm::OnClicked( ui::EventArgs* arg )
{
	std::wstring name = arg->pSender->GetName();
	if(name == L"btn_search")
	{
		std::string tid;
		{
			std::wstring str = GetRichText(re_tid_);
			StringHelper::Trim(str);
			if( str.empty() )
			{
				error_tip_->SetText(L"群号不可为空");
				error_tip_->SetVisible(true);

				btn_search_->SetEnabled(false);
				return false;
			}
			tid = nbase::UTF16ToUTF8(str);
		}
		tid_ = tid;
		btn_search_->SetEnabled(false);

		nim::Team::QueryTeamInfoOnlineAsync(tid_, nbase::Bind(&TeamSearchForm::OnGetTeamInfoCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
	else if(name == L"btn_return_search")
	{
		label_group_->SetVisible(false);
		btn_apply_->SetEnabled(true);

		GotoPage(TSP_SEARCH);
	}
	else if(name == L"btn_apply")
	{
		nim::Team::ApplyJoinAsync(tid_, "", nbase::Bind(&TeamSearchForm::OnApplyJoinCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

		GotoPage(TSP_APPLY_OK);
	}
	else if(name == L"btn_apply_ok")
	{
		this->Close();
	}
	return false;
}

void TeamSearchForm::OnGetTeamInfoCb(nim::NIMResCode res_code, nim::NIMNotificationId notification_id, const std::string& tid, const std::string& result)
{
	QLOG_APP(L"search team: {0}") <<result;
	
	if (res_code == 200)
	{
		ShowTeamInfo(result);
		GotoPage(TSP_APPLY);
	}
	else
	{
		error_tip_->SetText(L"群不存在");
		error_tip_->SetVisible(true);
	}
}

void TeamSearchForm::OnApplyJoinCb(nim::NIMResCode res_code, nim::NIMNotificationId notification_id, const std::string& tid, const std::string& result)
{
	QLOG_APP(L"apply join: {0}") <<result;
	
	switch (res_code)
	{
	case nim::kNIMResTeamAlreadyIn:
	{
		re_apply_->SetText(nbase::StringPrintf(L"已经在群里", tname_.c_str()));
	}
	break;
	case nim::kNIMResSuccess:
	{
		nbase::ThreadManager::PostTask(kThreadUI, nbase::Bind(TeamCallback::OnTeamEventCallback, nim::kNIMResSuccess, nim::kNIMNotificationIdLocalApplyTeam, tid, result));
		re_apply_->SetText(nbase::StringPrintf(L"群 %s 管理员同意了你的加群请求", tname_.c_str()));
	}
	break;
	case nim::kNIMResTeamApplySuccess:
		re_apply_->SetText(L"你的加群请求已发送成功，请等候群主/管理员验证");
		break;
	default:
	{
		re_apply_->SetText(nbase::StringPrintf(L"群 %s 管理员拒绝了你的加群请求", tname_.c_str()));
	}
	break;
	}
}
}