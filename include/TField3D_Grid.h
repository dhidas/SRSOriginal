#ifndef GUARD_TField3D_Grid_h
#define GUARD_TField3D_Grid_h
////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <dhidas@bnl.gov>
//
// Created on: Tue Sep 20 07:55:00 EDT 2016
//
////////////////////////////////////////////////////////////////////

#include "TField.h"

#include <string>
#include <vector>

class TField3D_Grid : public TField
{

  public:
    TField3D_Grid ();
    TField3D_Grid (std::string const&, std::string const& FileFormat = "OSCARS", TVector3D const& Rotations = TVector3D(0, 0, 0), TVector3D const& Translation = TVector3D(0, 0, 0), char const CommentChar = '#');
    ~TField3D_Grid ();

    double GetFx (double const, double const, double const) const;
    double GetFy (double const, double const, double const) const;
    double GetFz (double const, double const, double const) const;
    TVector3D GetF (double const, double const, double const) const;
    TVector3D GetF (TVector3D const&) const;

    size_t GetIndex (size_t const, size_t const, size_t const) const;

    double GetHeaderValue (std::string const&) const;
    double GetHeaderValueSRW (std::string const&, const char CommentChar = '#') const;

    void ReadFile         (std::string const&, TVector3D const& Rotations = TVector3D(0, 0, 0), TVector3D const& Translation = TVector3D(0, 0, 0), char const CommentChar = '#');
    void ReadFile_SRW     (std::string const&, TVector3D const& Rotations = TVector3D(0, 0, 0), TVector3D const& Translation = TVector3D(0, 0, 0), char const CommentChar = '#');
    void ReadFile_SPECTRA (std::string const&, TVector3D const& Rotations = TVector3D(0, 0, 0), TVector3D const& Translation = TVector3D(0, 0, 0), char const CommentChar = '#');

    enum TField3D_Grid_DIMX {
      kDIMX_X,
      kDIMX_Y,
      kDIMX_Z,
      kDIMX_XY,
      kDIMX_XZ,
      kDIMX_YZ,
      kDIMX_XYZ
    };



  private:
    // Dimension and position data
    int    fNX;
    int    fNY;
    int    fNZ;
    double fXStart;
    double fYStart;
    double fZStart;
    double fXStep;
    double fYStep;
    double fZStep;
    double fXStop;
    double fYStop;
    double fZStop;

    bool fHasX;
    bool fHasY;
    bool fHasZ;
    int  fXDIM;

    TField3D_Grid_DIMX fDIMX;

    // Rotations and Translations.  Field is stored rotated, point must be rotated into grid
    // coordinate system before asking for field, but remember field is already rotated.
    TVector3D fRotated;
    TVector3D fTranslation;

    // Field data
    std::vector<TVector3D> fData;

};



















#endif
