#ifndef GUARD_TVector3D_h
#define GUARD_TVector3D_h
////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <dhidas@bnl.gov>
//
// Created on: Fri Mar 25 10:24:13 EDT 2016
//
////////////////////////////////////////////////////////////////////





class TVector3D
{
  public:
    TVector3D ();
    TVector3D (double const&, double const&, double const&);
    ~TVector3D ();

    double GetX () const;
    double GetY () const;
    double GetZ () const;

    void SetX (double const&);
    void SetY (double const&);
    void SetZ (double const&);
    void SetXYZ (double const&, double const&, double const&);

    double Mag () const;
    double Mag2 () const;
    double Dot (TVector3D&) const;
    TVector3D Cross (TVector3D&) const;
    TVector3D UnitVector () const;


    // Operators
    TVector3D  operator  + (TVector3D const&) const;
    TVector3D  operator  - (TVector3D const&) const;
    TVector3D  operator  * (double const&) const;
    TVector3D  operator  / (double const&) const;
    TVector3D  operator  - ();
    TVector3D& operator  = (TVector3D const&);
    TVector3D& operator += (double const&);
    TVector3D& operator -= (double const&);
    TVector3D& operator *= (double const&);
    TVector3D& operator /= (double const&);
    bool       operator == (TVector3D const&) const;
    bool       operator != (TVector3D const&) const;

  private:
    double fX;
    double fY;
    double fZ;

};







#endif