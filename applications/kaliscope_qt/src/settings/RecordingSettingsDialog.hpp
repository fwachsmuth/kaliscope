#ifndef _RECORDINGSETTINGSDIALOG_HPP_
#define	_RECORDINGSETTINGSDIALOG_HPP_

#include "ui_RecordingSettingsDialog.hpp"

#include <kali-core/settingsTools.hpp>

#include <mvp-player-core/Settings.hpp>

#include "TablePluginItem.hpp"

#include <tuttle/common/utils/global.hpp>
#include <tuttle/host/Graph.hpp>

namespace kaliscope
{
namespace gui
{
namespace qt
{

static const std::string kKaliscopeDefaultPipelineSettingsFilename( ".kaliDefaultPipeline.json" );
static const std::string kPresetsDirectory( "presets" );

/**
 * @brief recording dialog settings
 * allow the user to edit the processing pipeline
 */
class RecordingSettingsDialog : public QDialog
{
    Q_OBJECT
private:
    typedef RecordingSettingsDialog This;
    typedef QDialog Parent;
public:
    RecordingSettingsDialog( QWidget *parent = nullptr );
    virtual ~RecordingSettingsDialog();

    const mvpplayer::Settings & pipelineSettings() const
    { return _pipelineSettings; }

private Q_SLOTS:
    void removePluginSelection();
    void rebuildPipelineSettings();
    void addPlugin();
    void editPluginParams( QListWidgetItem * item );
    void loadPreset( const int index );
    void loadConfig();
    void saveConfig();
    void browseInput();
    void browseOutput();
    void recomputeNbImages();
    void setConfigPathParamsFromSettings();

private:
    TablePluginItem * addPlugin( const tuttle::host::ofx::imageEffect::OfxhImageEffectPlugin & plugin, const mvpplayer::Settings & settings );
    void buildPipelineFrom( const mvpplayer::Settings & pipelineSettings );
    QWidget* buildPluginWidgetFrom( TablePluginItem *plugItem );
    void accept();
    bool eventFilter( QObject* sender, QEvent* event );
    void loadPresetItems();
    void setConfigPaths();

private:
    std::map<int, mvpplayer::Settings> _presets;                    ///< Presets
    Ui::RecordingSettingsDialog widget;
    mvpplayer::Settings _pipelineSettings;                          ///< The pipeline global settings
};

}
}
}

#endif
