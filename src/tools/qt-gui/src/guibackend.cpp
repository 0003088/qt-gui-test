#include "guibackend.hpp"

#include <markdowndocument.h>
#include <discountmarkdownconverter.h>
#include <backends.hpp>
#include <vector>
#include <string>
#include <QDebug>
#include <QRegExp>

using namespace std;
using namespace kdb;
using namespace kdb::tools;

GUIBackend::GUIBackend(QObject *parent) :
	QObject(parent)
{
}

GUIBackend::GUIBackend(const GUIBackend &other)
{
	Q_UNUSED(other);
}

void GUIBackend::createBackend(const QString &mountpoint)
{
	m_backend = new Backend();

	Key parentKey(Backends::mountpointsPath, KEY_END);

	try
	{
		m_kdb.get(m_mountConf, parentKey);
	}
	catch(KDBException ex)
	{
		emit showMessage(tr("Error"), tr("Could not read from configuration."), QString(ex.what()));
	}

	Key cur = m_mountConf.lookup(parentKey);

	if (!cur)
	{
		m_mountConf.append ( *Key(Backends::mountpointsPath,
								  KEY_COMMENT, "Below are the mountpoints.",
								  KEY_END));
		m_mountConf.rewind();
	}

	try
	{
		m_backend->setMountpoint(Key(mountpoint.toStdString(), KEY_CASCADING_NAME, KEY_END), m_mountConf);
	}
	catch(MountpointInvalidException ex)
	{
		emit showMessage(tr("Error"), tr("The provided mount point is invalid."), ex.what());
	}
	catch(MountpointAlreadyInUseException ex)
	{
		emit showMessage(tr("Error"), tr("The provided mount point is one of the already used cascading names."), ex.what());
	}

	m_name = mountpoint;
	m_name.replace("/", "_");
}

void GUIBackend::addPath(const QString &path)
{
	try
	{
		m_backend->checkFile(path.toStdString());
	}
	catch(FileNotValidException ex)
	{
		emit showMessage(tr("Error"), tr("The file you have entered is not valid."), ex.what());
	}
	catch(MissingSymbol ex)
	{
		emit showMessage(tr("Error"), tr("Could not add file."), ex.what());
	}

	std::string configPath = Backends::getConfigBasePath(m_name.toStdString());

	m_mountConf.append(*Key(configPath,
							KEY_VALUE, "",
							KEY_COMMENT, "This is a configuration for a backend, see subkeys for more information",
							KEY_END));
	configPath += "/path";

	QByteArray pathArr = path.toLocal8Bit();

	m_mountConf.append (*Key(configPath,
							 KEY_VALUE, pathArr.data(),
							 KEY_COMMENT, "The path for this backend. Note that plugins can override that with more specific configuration.",
							 KEY_END));
}

void GUIBackend::addPlugin(QString name, QStringList config)
{
	name.chop(name.length() - name.indexOf("[") + 1);

	KDB kdb;
	KeySet globalConf;
	KeySet pluginConf;

	try
	{
		kdb.get(globalConf, "/");
	}
	catch(KDBException const& ex)
	{
		//TODO: emit proper Error
		qDebug() << "ERROR";
	}

	foreach(QString s, config)
	{
		Key k = globalConf.lookup(s.toStdString());
		pluginConf.append(k.dup());
	}

	try
	{
		m_backend->addPlugin(name.toStdString(), pluginConf);
	}
	// TODO: if exceptions are not handled differently, catching
	// base class is enough
	// Unfortunately if only the base class is catched, it is not possible to determine the cause of the
	// exception, on every error the text will be: "When you read this, that means there was something wrong with the plugin."
	catch(TooManyPlugins ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(OrderingViolation ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(ConflictViolation ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(ReferenceNotFound ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(MissingNeeded ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(MissingSymbol ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(SymbolMismatch ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(SymbolDuplicate ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(StoragePlugin ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(ResolverPlugin ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(PluginNoContract ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(PluginNoInfo ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(VersionInfoMismatch ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
	catch(PluginCheckException ex)
	{
		emit showMessage(tr("Error"), tr("Could not add plugin \"%1\".").arg(name), ex.what());
	}
}

void GUIBackend::serialise()
{
	Key rootKey (Backends::mountpointsPath, KEY_END);

	try
	{
		m_backend->serialise(rootKey, m_mountConf);
	}
	catch(ToolException ex)
	{
		emit showMessage(tr("Error"), tr("Could not serialise backend."), ex.what());
	}

	try
	{
		m_kdb.set(m_mountConf, rootKey);
	}
	catch (kdb::KDBException const& ex)
	{
		emit showMessage(tr("Error"), tr("Could not write backend to configuration."), ex.what());
	}
}

bool GUIBackend::validated()
{
	return m_backend->validated();
}

void GUIBackend::deleteBackend()
{
	delete m_backend;
}

QString GUIBackend::mountPoints() const
{
	Key parentKey(Backends::mountpointsPath, KEY_END);
	KeySet mountConf;
	KDB kdb (parentKey);

	try
	{
		kdb.get(mountConf, parentKey);
	}
	catch(KDBException const& ex)
	{
		//TODO: emit proper Error
		qDebug() << "ERROR";
	}

	QStringList mountPoints;
	mountPoints.append("system/elektra");

	mountConf.rewind();

	Key cur;

	while ((cur = mountConf.next()))
	{
		if (cur.getBaseName() == "mountpoint")
		{
			if (cur.getString().at(0) == '/')
			{
				mountPoints.append(QString::fromStdString("user" + cur.getString()));
				mountPoints.append(QString::fromStdString("system" + cur.getString()));
			}
			else
			{
				mountPoints.append(QString::fromStdString(cur.getString()));
			}

		}

	}
	return mountPoints.join(", ");
}

QString GUIBackend::pluginInfo(QString pluginName) const
{
	Modules modules;
	KeySet info;
	QString infoString;

	pluginName.chop(pluginName.length() - pluginName.indexOf("[") + 1);

	PluginPtr plugin = modules.load(pluginName.toStdString());

	info = plugin->getInfo();

	Key root;
	root.setName(std::string("system/elektra/modules/") + plugin->name() + "/infos");
	Key k = info.lookup(root);

	if (k)
	{
		while ((k = info.next()) && k.getDirName() == root.getName())
		{
			infoString.append(QString::fromStdString(k.getBaseName()) + ": " + QString::fromStdString(k.getString()) + "\n\n");
		}
	}
	else
		infoString.append(tr("No information found."));

	DiscountMarkdownConverter dmc;
	infoString = dmc.renderAsHtml(dmc.createDocument(infoString, DiscountMarkdownConverter::NoImagesOption));

	return infoString;
}

QStringList GUIBackend::availablePlugins(bool includeStorage, bool includeResolver) const
{
	QStringList availPlugins;
	Modules modules;
	PluginPtr ptr;
	QString type;

	vector<string> pluginVector = listAllAvailablePlugins();

	foreach(string s, pluginVector){
		try
		{
			ptr = modules.load(s);
		}
		catch(NoPlugin ex)
		{
			break;
		}

		ptr->loadInfo();
		type = QString::fromStdString(ptr->lookupInfo("provides"));

		if(!((!includeStorage && type == "storage") || (!includeResolver && type == "resolver"))){
		  availPlugins.append(QString::fromStdString(s) + QString::fromStdString(" [%1]").arg(type));
		}
	}

	availPlugins.sort();

	return availPlugins;
}

QStringList GUIBackend::nameFilters()
{
	QStringList nameFilters;
	QStringList plugins = availablePlugins(true, false);

	plugins = plugins.filter("[storage]");
	plugins.replaceInStrings(QRegExp("\\s\\[\\w*\\]"), "");

	nameFilters.append("DUMP (*.ecf)");

	if(plugins.contains("ini"))
		nameFilters.append("INI (*.ini)");
	if(plugins.contains("ni"))
		nameFilters.append("NICKEL (*.ini)");
	if(plugins.contains("simpleini"))
		nameFilters.append("SIMPLEINI (*.ini)");
	if(plugins.contains("fstab"))
		nameFilters.append("FSTAB (*.*)");
	if(plugins.contains("hosts"))
		nameFilters.append("HOSTS (*.*)");
	if(plugins.contains("line"))
		nameFilters.append("LINE (*.*)");
	if(plugins.contains("regexstore"))
		nameFilters.append("REGEXSTORE (*.*)");
	if(plugins.contains("tcl"))
		nameFilters.append("TCL (*.tcl)");
	if(plugins.contains("xmltool"))
		nameFilters.append("XMLTOOL (*.xml)");
	if(plugins.contains("uname"))
		nameFilters.append("UNAME (*.*)");
	if(plugins.contains("yajl"))
		nameFilters.append("YAJL (*.json)");
	if(plugins.contains("constants"))
		nameFilters.append("CONSTANTS (*.*)");

	nameFilters.sort();

	return nameFilters;
}
