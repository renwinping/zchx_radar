#include "boost/bind.hpp"

#include "Messages/RadarConfig.h"
#include "Messages/TSPI.h"
#include "Messages/Track.h"

#include "TrackMaintainer.h"
//#include "TrackMaintainer_defaults.h"
#include "zchxRadarUtils.h"

#include "QtCore/QString"
#include "QtCore/QVariant"
#include "common.h"
#include <QDateTime>

using namespace ZCHX;
using namespace ZCHX::Messages;
using namespace ZCHX::Algorithms;

TrackMaintainer::TrackMaintainer() :
    enabled_(/*kDefaultEnabled*/true),
    hitsBeforePromote_(/*kDefaultHitsBeforePromote*/1000),
    missesBeforeDrop_(/*kDefaultMissesBeforeDrop*/2000)
{
    ;
}

//bool
//TrackMaintainer::startup()
//{
//    registerProcessor<TrackMaintainer, Track>("corrected", &TrackMaintainer::processInput);
//    setAlarm(5); // set an alarm to go off every 5 seconds
//    return registerParameter(enabled_) && registerParameter(missesBeforeDrop_) &&
//           registerParameter(hitsBeforePromote_) && Super::startup();
//}

bool
TrackMaintainer::reset()
{
    trackDatabase_.clear();
    return true;
}

/** This method will get called whenever the timer for this Algorithm goes off
 */
void
TrackMaintainer::processAlarm()
{
    LOG_FUNC_DBG << "checking database - trks " << trackDatabase_.size() << endl;
    checkDatabase();
}

//bool
//TrackMaintainer::shutdown()
//{
//    trackDatabase_.clear();
//    return Super::shutdown();
//}

bool
TrackMaintainer::processInput(const Messages::Track::Ref& msg)
{
    //LOG_FUNC_DBG << msg->headerPrinter() << endl;
    //LOG_FUNC_DBG << msg->dataPrinter() << endl;

    if (msg->getFlags() == Track::kNew || msg->getFlags() == Track::kCorrected) { updateDatabase(msg); }

    return true;
}

void
TrackMaintainer::updateDatabase(const Messages::Track::Ref& msg)
{
    LOG_FUNC_DBG << "track database has " << trackDatabase_.size() << " entries" << endl;

    // Find existing entry in map or create a new one
    //
    Mapping::iterator it = trackDatabase_.find(msg->getTrackNumber());
    if (it == trackDatabase_.end()) {
        LOG_FUNC_DBG << "new entry for track num " << msg->getTrackNumber() << endl;
        it = trackDatabase_.insert(Mapping::value_type(msg->getTrackNumber(), TrackMsgVector())).first;
    }

    // Add the given message to the track vector
    //
    it->second.push_back(msg);

    // Update the offset between message time and wall time.
    //
    //Time::TimeStamp now = Time::TimeStamp::Now();
    epoch_ = QDateTime::currentMSecsSinceEpoch() - msg->getExtractionTime();

    LOG_FUNC_DBG << "epoch " << epoch_ << endl;
}

/** This method checks for tracks that need to be promoted from tentative to firm and also for tracks that have
    not been updated recently and need to be dropped. If a track needs to be promoted or dropped, this method
    will send out a Track message to this effect.

    This method should be careful not to use current time to compare against
    because we need to be able to accomodate data playback, which may contain
    data with timestamps that are far in the past.
*/

void
TrackMaintainer::checkDatabase()
{

    LOG_FUNC_DBG << "track database has " << trackDatabase_.size() << " entries" << endl;

    double dropLimit = RadarConfig(0).getRotationDuration() * missesBeforeDrop_;
    LOG_FUNC_DBG << "drop duration " << dropLimit << endl;

    // Loop through the data base.
    //
    Mapping::iterator it = trackDatabase_.begin();
    while (it != trackDatabase_.end()) {
        LOG_FUNC_DBG << "track database entry " << it->first << endl;

        const TrackMsgVector& tracks(it->second);
        const Messages::Track::Ref& track(tracks.back());

        Messages::Track::Ref report;

        // Check to see if track is promotable
        //
        if (track->getType() == Track::kTentative) {
            LOG_FUNC_DBG << "tentative track with " << tracks.size()
                     << " msgs. Threshold= " << hitsBeforePromote_ << endl;
            if (tracks.size() >= hitsBeforePromote_) {
                LOG_FUNC_DBG << "track should be promoted " << endl;
                report = Track::Make("TrackMaintainer", track);
                report->setFlags(Track::kPromoted);
            }
        }

        // Check to see if track should be dropped
        //
        double now = QDateTime::currentMSecsSinceEpoch();
        if ((now - (epoch_ + track->getExtractionTime())) > dropLimit) {
            LOG_FUNC_DBG << "dropping track " << it->first << endl;

            report = Track::Make("TrackMaintainer", track);
            report->setFlags(Track::kDropping);

            // NOTE: erasing the current item only affects the current iterator but no others, so we
            // post-increment so that we have a valid iterator to the next element after the erasure.
            //
            trackDatabase_.erase(it++);
        } else {
            // Not deleting track, so safe to advance the iterator to the next one.
            //
            ++it;
        }

        // If we created a report above, send it out.
        //
        if (report) {
            //send(report);

            std::string flag;
            switch (report->getFlags()) {
            case Track::kDropping: flag = " dropping"; break;
            case Track::kNew: flag = "new"; break;
            case Track::kPromoted: flag = "promoted"; break;
            case Track::kNeedsPrediction: flag = "needs prediction"; break;
            case Track::kNeedsCorrection: flag = "needs correction"; break;
            case Track::kPredicted: flag = "predicted"; break;
            case Track::kCorrected: flag = "corrected"; break;
            default: break;
            };

            std::string type;
            switch (report->getType()) {
            case Track::kTentative: type = "tentative"; break;
            case Track::kConfirmed: type = "confirmed"; break;
            default: break;
            };

            //LOG_FUNC_DBG << "maintained track: " << report->getTrackNumber() << " flag: " << flag << " type: " << type<< endl;
        }
    }
}


