/***************************************************************************

Copyright (c) Microsoft Corporation. All rights reserved.
This code is licensed under the Visual Studio SDK license terms.
THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.

***************************************************************************/

#pragma once

/*

The MenuAndCommandsPackage class implements a simple VS package that exposes a set of commands. 
This description will focus on the way the commands are exposed and implemented, not on the package
itself; for more information on writing packages using VSL look at the Reference.Package sample.

In order to expose commands (menu or toolbar items) a package must
1.	Tell the shell that the commands exist and where to place them inside the menu or toolbar 
	structure.
2.	Handle the execution and status of the commands.

The first task is done by embedding a binary resource with the description of the commands inside 
the UI dll of the package, then calling 'devenv /setup' to force the shell to rebuild the menu and
toolbar cache; this step is needed only when the menu structure is changing (e.g. during the 
development phase of your package or when it is installed on the target machine). The binary file 
with the description of the commands is generated by the VSCT compiler starting from a .VSCT file.
For more details about this file look at MenuAndCommands.vsct in the MenuAndCommandUI project.

To handle the status and the execution of a command the shell requires that the package 
implements the IOleCommandTarget interface; this interface has two methods.  The first method is 
QueryStatus, which is used to retrieve information about the command like if it is visible or its 
text.  The second method is Exec, which is called by the shell when the user executes the command
(e.g. selecting a menu item or clicking on a toolbar button).

To help with the implementation of IOleCommandTarget, VSL provide the IOleCommandTargetImpl 
template class; this class implements IOleCommandTarget and uses a map of objects derived from 
CommandHandlerBase to handle the commands.

This example shows how to use IOleCommandTargetImpl to define the command handlers using the VSL
command map macros.

*/

#include <atlstr.h>
#include <VSLCommandTarget.h>

#include "Guids.h"
#include "Resource.h"

#include "..\MenuAndCommandsUI\Resource.h"
#include "..\MenuAndCommandsUI\CommandIds.h"

class ATL_NO_VTABLE MenuAndCommandsPackage : 
	// CComObjectRootEx and CComCoClass are used to implement a non-thread safe COM object, and 
	// a partial implementation for IUknown (the COM map below provides the rest).
    public CComObjectRootEx<CComSingleThreadModel>,
     public CComCoClass<MenuAndCommandsPackage, &CLSID_MenuAndCommands>,
	// Provides the implementation for IVsPackage to make this COM object into a VS Package.
    public IVsPackageImpl<MenuAndCommandsPackage, &CLSID_MenuAndCommands>, 
	// Provides the implementation for IOleCommandTarget to make this VS Package into a VS Command
	// Target, so it can handle menu item and toolbar item events.
	public IOleCommandTargetImpl<MenuAndCommandsPackage>,
	// Provides consumers of this object with the ability to determine which interfaces support
	// extended error information.
	public VSL::ISupportErrorInfoImpl<
		InterfaceSupportsErrorInfoList<IVsPackage,
		InterfaceSupportsErrorInfoList<IOleCommandTarget> > >
{

// Provides a portion of the implementation of IUnknown, in particular the list of interfaces
// the BasicPackage object will support via QueryInterface
BEGIN_COM_MAP(MenuAndCommandsPackage)
	COM_INTERFACE_ENTRY(IVsPackage)
	COM_INTERFACE_ENTRY(IOleCommandTarget)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// COM objects typically should not be cloned, and this prevents cloning by declaring the 
// copy constructor and assignment operator private (NOTE:  this macro includes the decleration of
// a private section, so everything following this macro and preceding a public or protected 
// section will be private).
VSL_DECLARE_NOT_COPYABLE(MenuAndCommandsPackage)

protected:
	// Here we define a simple class derived from the CommandHandlerBase defined in the VSL in order
	// to create a command handler that counts the number of times the user clicks on the menu item
	// and changes the text to show the counter.
	class DynamicTextCommandHandler : public CommandHandler
	{

	// The ability to clone this class is not necessary, so prevent it (NOTE:  this macro includes 
	// the decleration of a private section, so everything following this macro and preceding a 
	// public or protected  section will be private).
	VSL_DECLARE_NOT_COPYABLE(DynamicTextCommandHandler)

	private:
		DWORD		m_dwClickCount;
		CStringW	m_strOriginalText;
	public:
		DynamicTextCommandHandler(const CommandId& id, _In_opt_ wchar_t* szText) :
		    CommandHandler(id, NULL, NULL, OLECMDF_SUPPORTED | OLECMDF_ENABLED, szText),
			m_dwClickCount(0),
			m_strOriginalText(szText)
		{}

		// This function is called by IOleCommandTargetImpl when the command is executed.
		// Here we want to format the text adding the "(Clicked %d times)" to the text passed
		// to the constructor of the object
		void Exec(MenuAndCommandsPackage* /*target*/, DWORD /*flags*/, VARIANT* /*pIn*/, VARIANT* /*pOut*/)
		{
			++m_dwClickCount;
			// Get the format string from the resources.
			CStringW strFormat;
			VSL_CHECKBOOL_GLE(strFormat.LoadString(IDS_COMMAND_TEXT_FORMAT));

			// Format the text using the format string we get from the resources.
			// Notice that we call GetText() from the base class to get the string that was used
			// to build the object.
			GetText().Format(strFormat, m_strOriginalText, m_dwClickCount);
		}

        void ResetClickCount()
        {
            m_dwClickCount = 0;
            GetText() = m_strOriginalText;
        }
	};

public:
	MenuAndCommandsPackage()
	{
	}
	
	~MenuAndCommandsPackage()
	{
	}

	// This method will be called after IVsPackage::SetSite is called with a valid site
	void PostSited(IVsPackageEnums::SetSiteResult /*result*/)
	{
		// Initialize the output window utility class
		m_OutputWindow.SetSite(GetVsSiteCache());
	}

	// This function provides the error information if it is not possible to load
	// the UI dll. It is for this reason that the resource IDS_E_BADINSTALL must
	// be defined inside this dll's resources.
	static const LoadUILibrary::ExtendedErrorInfo& GetLoadUILibraryErrorInfo()
	{
		static LoadUILibrary::ExtendedErrorInfo errorInfo(IDS_E_BADINSTALL);
		return errorInfo;
	}

	// DLL is registered with VS via a pkgdef file. Don't do anything if asked to
	// self-register.
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		return S_OK;
	}

// Defines the command handlers. IOleCommandTargetImpl will use these handlers to implement
// IOleCommandTarget.
VSL_BEGIN_COMMAND_MAP()
	// Every command is identified by the shell using a GUID/DWORD pair, so every the definition of
	// commands must contain this information.

	// The following four definitions of command map entries are the more common: we define a 
	// GUID/DWORD pair to identify the command and a callback for the command execution. We let 
	// the CommandHandler's default implementation to handle the status of the commands.
    VSL_COMMAND_MAP_ENTRY(CLSID_MenuAndCommandsCmdSet, cmdidMyCommand, NULL, CommandHandler::ExecHandler(&MenuAndCommandsPackage::MenuCommandCallback))
	VSL_COMMAND_MAP_ENTRY(CLSID_MenuAndCommandsCmdSet, cmdidMyGraph, NULL, CommandHandler::ExecHandler(&MenuAndCommandsPackage::GraphCommandCallback))
	VSL_COMMAND_MAP_ENTRY(CLSID_MenuAndCommandsCmdSet, cmdidMyZoom, NULL, CommandHandler::ExecHandler(&MenuAndCommandsPackage::ZoomCommandCallback))
	VSL_COMMAND_MAP_ENTRY(CLSID_MenuAndCommandsCmdSet, cmdidDynVisibility1, NULL, CommandHandler::ExecHandler(&MenuAndCommandsPackage::MenuVisibilityCallback))

	// This definition of a command map entry is very similar to the four above; however, this 
	// definition sets the default visibility for the command to invisible.
	VSL_COMMAND_MAP_ENTRY_WITH_FLAGS(CLSID_MenuAndCommandsCmdSet, cmdidDynVisibility2, NULL, CommandHandler::ExecHandler(&MenuAndCommandsPackage::MenuVisibilityCallback), OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_INVISIBLE)

	// This definition of a command map entry is different because here we provide the id of the 
	// command as parameters for the contructor of a DynamicTextCommandHandler instance.
	VSL_COMMAND_MAP_CLASS_ENTRY(DynamicTextCommandHandler, (CommandId(CLSID_MenuAndCommandsCmdSet, cmdidDynamicTxt), L"C++ Text Changes"))
// Terminate the definition of the command map
VSL_END_VSCOMMAND_MAP()

private:

	////////////////////////////////////////////////////////////////////////////////
	// Callback functions used by the command handlers.
	void MenuCommandCallback(CommandHandler* /*pSender*/, DWORD /*flags*/, VARIANT* /*pIn*/, VARIANT* /*pOut*/)
	{
		CStringW strMessage;
		VSL_CHECKBOOL_GLE(strMessage.LoadString(IDS_COMMAND_CALLBACK));
		m_OutputWindow.OutputMessageWithPreAndPostBarsOfEquals(strMessage);
	}

	void GraphCommandCallback(CommandHandler* /*pSender*/, DWORD /*flags*/, VARIANT* /*pIn*/, VARIANT* /*pOut*/)
	{
		CStringW strMessage;
		VSL_CHECKBOOL_GLE(strMessage.LoadString(IDS_GRAPH_CALLBACK));
		m_OutputWindow.OutputMessageWithPreAndPostBarsOfEquals(strMessage);
	}

	void ZoomCommandCallback(CommandHandler* /*pSender*/, DWORD /*flags*/, VARIANT* /*pIn*/, VARIANT* /*pOut*/)
	{
		CStringW strMessage;
		VSL_CHECKBOOL_GLE(strMessage.LoadString(IDS_ZOOM_CALLBACK));
		m_OutputWindow.OutputMessageWithPreAndPostBarsOfEquals(strMessage);
	}

	void MenuVisibilityCallback(CommandHandler* pSender, DWORD flags, VARIANT* pIn, VARIANT* pOut);
	// End of the callback functions.
	////////////////////////////////////////////////////////////////////////////////

	VsOutputWindowUtilities<> m_OutputWindow;
};

// This exposes MenuAndCommandsPackage for instantiation via DllGetClassObject; however, an instance
// can not be created by CoCreateInstance, as MenuAndCommandsPackage is specfically registered with
// VS, not the the system in general.
OBJECT_ENTRY_AUTO(CLSID_MenuAndCommands, MenuAndCommandsPackage)