#include "pathi.h"

using namespace Path;

int PATH_milliseconds()
{
    unsigned int ms = 0;

    if (IPathToReal::realimp)
        ms = IPathToReal::realimp->GetMilliseconds();

    return ms;
}

int PATHI_switchproject(int p, int idflags)
{
    if (p >= 0 && p < PATH_MAX_PROJECTS && pfstates[p] && pfstates[p]->pmap)
    {
        int voiceflags = idflags & PATH_ALL_VOICES;

        if (!voiceflags || (pfstates[p]->idflags & voiceflags))
        {
            int projectflags = idflags & PATH_ALL_PROJECTS;

            if (pfstates[p]->idflags & projectflags)
            {
                pfstate = pfstates[p];
                // El original relee idflags tras cambiar de proyecto: con el compilador de path (SN v1.83,
                // sin la regla fixed_scalar_and_varying_struct_p) sale asi escrito de forma natural.
                return pfstate->idflags;
            }
        }
    }

    return 0;
}

int PATHI_switchvoice(unsigned int voiceflags)
{
    int p = 0;
    int hasproject = (voiceflags & PATH_ALL_PROJECTS) != 0;

    for (; p < PATH_MAX_PROJECTS; p++)
    {
        int projectflag = 0;

        if (!hasproject)
            projectflag = PATH_PROJECT(p);

        if (PATHI_switchproject(p, voiceflags | projectflag))
            return 1;
    }

    return 0;
}

void PATHI_sortprojects()
{
    int p1;
    int p2;
    PATHFINDERSTATE pfstate1;

    for (p1 = 0; p1 < PATH_MAX_PROJECTS; p1++)
    {
        if (!pfstates[p1])
            continue;

        for (p2 = p1 + 1; p2 < PATH_MAX_PROJECTS; p2++)
        {
            if (!pfstates[p2])
                continue;

            if (pfstates[p1]->idflags > pfstates[p2]->idflags)
            {
                pfstate1 = *pfstates[p1];
                *pfstates[p1] = *pfstates[p2];
                *pfstates[p2] = pfstate1;
            }
        }
    }
}

void PATHI_serviceproject()
{
    PATHTRACK *track;
    int timeremaining;
    unsigned int t;
    unsigned int interval;
    char trackplaying;
    char intimer = (bankservice == 'B');

    if (intimer)
        interval = pfstate->timerinterval;
    else
        interval = pfstate->taskinterval;

    if (!intimer)
        PATHI_serviceeventqueue();

    for (t = 0; t < PATH_MAX_TRACKS; t++)
    {
        track = pfstate->track[t];
        if (!track)
            continue;

        if (!track->trackimp)
            continue;

        trackplaying = (track->node >= 0 && track->entryinfo != 0);

#ifdef EA_PLATFORM_XENON
        if (trackplaying && track->volumefade.fadenum >= 0)
            PATHI_setfadevolume(track);
        if (trackplaying && track->sfxsendfade.fadenum >= 0)
            PATHI_setsfxfadevolume(track);
        if (trackplaying && track->drylevelfade.fadenum >= 0)
            PATHI_setdrylevelfadevolume(track);
        if (trackplaying && track->pitchfade.fadenum >= 0)
            PATHI_setpitchfadevolume(track);
        if (trackplaying && track->stretchfade.fadenum >= 0)
            PATHI_setstretchfadevolume(track);
#else
        if (trackplaying)
        {
            if (track->volumefade.fadenum >= 0)
                PATHI_setfadevolume(track);
            if (track->sfxsendfade.fadenum >= 0)
                PATHI_setsfxfadevolume(track);
            if (track->drylevelfade.fadenum >= 0)
                PATHI_setdrylevelfadevolume(track);
            if (track->pitchfade.fadenum >= 0)
                PATHI_setpitchfadevolume(track);
            if (track->stretchfade.fadenum >= 0)
                PATHI_setstretchfadevolume(track);
        }
#endif

        if (track->pauseat && track->pauseat <= milliseconds + interval / 2)
        {
            track->trackimp->Pause(1);
            track->pauseat = 0;
            track->paused = 1;
        }

        if (track->resumeat && track->resumeat <= milliseconds + interval / 2)
        {
            track->trackimp->Pause(0);
            track->resumeat = 0;
            track->paused = 0;
        }

        if (!intimer)
        {
            if (track->loadingsubbank >= 0)
                PATHI_subbankready(track, track->loadingsubbank);
        }

        if ((track->ramtrack != 0) != intimer)
            continue;

        if (!trackplaying)
            continue;

        if (track->paused)
            continue;

        if (!PATHI_readyfornewrequest(track))
            continue;

        timeremaining = PATHI_timeremaining(track);

        if (track->nextbeattime)
        {
            if (track->nextbeattime > milliseconds)
                continue;
        }
        else if (timeremaining >= (int)track->latency)
            continue;

        if (track->volumefade.fadeto == 0 && track->trackimp->GetVolume() == 0)
            track->volumefade.fadeto = -1;

        PATHI_seeknextnode(t);
    }

    while (!intimer && PATHI_serviceeventqueue())
        ;
}

void PATHI_service(char isbankservice)
{
    unsigned char p;

    if (paused)
        return;

    if (!PATHI_lock())
        return;

    bankservice = isbankservice;
    milliseconds = PATH_milliseconds();

    PATHI_statusall(0);

    for (p = 0; p < PATH_MAX_PROJECTS; p++)
    {
        if (PATHI_switchproject(p, PATH_ALL))
        {
            PATHI_getmastertrack();
            PATHI_serviceproject();
        }
    }

    PATHI_statusall(1);
    PATHI_unlock();
}

void PATHI_servicetask()
{
    PATHI_service(' ');
}

void PATHI_servicetimer()
{
    int timercb = PATH_milliseconds() / 10 * 10;

    lasttimercb = timercb;

    PATHI_service('B');
}
