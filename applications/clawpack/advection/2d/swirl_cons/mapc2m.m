function [xp,yp,zp] = mapc2m(xc,yc)

% map = 'nomap';
map = 'cart';   % brick
map = 'fivepatch';

% This domain should be in [0,2],[0,2]

shift = [0.5,0.5,0];

switch map
    case 'nomap'
        % Uses [ax,bx] x [ay,by] set in fclaw2d_defaults.ini
        % This is what is stored in the fort.q files.
        xp = xc;
        yp = yc;
        
    case 'cart'
        % (xc,yc) in [0,1]x[0,1]
        s = 0.005;
        [xc1,yc1,~] = mapc2m_brick(xc,yc,s);
        [xp,yp,~] = mapc2m_cart(xc1,yc1);
                
        
        xp = xp/2 + shift(1);
        yp = yp/2 + shift(2);
        
    case 'fivepatch'
        alpha = 0.5;
        [xp,yp,~] = mapc2m_fivepatch(xc,yc,alpha);
        b = getblocknumber();
        s = 0.005;
        switch b
            case 0
                yp = yp - s;
            case 1
                xp = xp - s;
            case 3
                xp = xp + s;
            case 4
                yp = yp + s;
        end
        xp = xp/2+shift(1);
        yp = yp/2+shift(2);
end
zp = 0*xp;




end