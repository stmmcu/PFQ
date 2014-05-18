--    Copyright (c) 2011-14, Nicola Bonelli
--    All rights reserved.
--
--    Redistribution and use in source and binary forms, with or without
--    modification, are permitted provided that the following conditions are met:
--
--    * Redistributions of source code must retain the above copyright notice,
--      this list of conditions and the following disclaimer.
--    * Redistributions in binary form must reproduce the above copyright
--      notice, this list of conditions and the following disclaimer in the
--      documentation and/or other materials provided with the distribution.
--    * Neither the name of University of Pisa nor the names of its contributors
--      may be used to endorse or promote products derived from this software
--      without specific prior written permission.
--
--    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
--    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
--    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
--    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
--    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
--    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
--    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
--    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
--    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
--    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
--    POSSIBILITY OF SUCH DAMAGE.
--
--

{-# LANGUAGE ExistentialQuantification #-}
{-# LANGUAGE NoMonomorphismRestriction #-}
{-# LANGUAGE ImpredicativeTypes #-}
{-# LANGUAGE Rank2Types #-}
{-# LANGUAGE GADTs #-}

{-# OPTIONS_GHC -fno-warn-unused-binds #-}

module Network.PFq.Lang
    (
        StorableContext(..),
        Arguments(..),
        Combinator(..),
        Predicate(..),
        Property(..),
        NetFunction(..),
        Serializable(..),
        FunDescr(..),
        FunType(..),
        Action,
        SkBuff,
        (>->),

    ) where


import Control.Monad.Identity
import Foreign.Storable


-- StorableContext

data StorableContext = forall a. (Show a, Storable a) => StorableContext a

instance Show StorableContext where
        show (StorableContext c) = show c


data Arguments = Empty | ArgData StorableContext | ArgFun Int | ArgDataFun StorableContext Int
                    deriving Show

-- Functional descriptor

data FunType = MonadicFun | HighOrderFun | PredicateFun | CombinatorFun | PropertyFun
                deriving (Show, Enum)

data FunDescr = FunDescr
                {
                    functionalType  :: FunType,
                    functionalSymb  :: String,
                    functionalArg   :: Arguments,
                    functionalLeft  :: Int,
                    functionalRight :: Int
                }
                deriving (Show)


relinkFunDescr :: Int -> Int -> FunDescr -> FunDescr
relinkFunDescr n1 n2 (FunDescr t name arg l r) =
        FunDescr t name arg (update n1 n2 l) (update n1 n2 r)
            where update n1 n2 x = if x == n1 then n2 else x

-- Serializable class

class Serializable a where
    serialize :: Int -> a -> ([FunDescr], Int)

-- Predicates and combinators

newtype Combinator = Combinator String

instance Show Combinator where
        show (Combinator "or")  = "|"
        show (Combinator "and") = "&"
        show (Combinator "xor") = "^"
        show (Combinator _)     = undefined

instance Serializable Combinator where
        serialize n (Combinator name) = ([FunDescr { functionalType  = CombinatorFun,
                                                     functionalSymb  = name,
                                                     functionalArg   = Empty,
                                                     functionalLeft  = -1,
                                                     functionalRight = -1 }], n+1)

data Property where
        Prop  :: String -> Property
        Prop1 :: forall a. (Show a, Storable a) => String -> a -> Property

instance Show Property where
        show (Prop  name)       = name
        show (Prop1 name a1)    = "(" ++ name ++ " " ++ show a1 ++ ")"

instance Serializable Property where
        serialize n (Prop name)   = ([FunDescr { functionalType  = PropertyFun,
                                                 functionalSymb  = name,
                                                 functionalArg   = Empty,
                                                 functionalLeft  = -1,
                                                 functionalRight = -1 }], n+1)

        serialize n (Prop1 name x) = ([FunDescr { functionalType  = PropertyFun,
                                                  functionalSymb  = name,
                                                  functionalArg   = ArgData $ StorableContext x,
                                                  functionalLeft  = -1,
                                                  functionalRight = -1 }], n+1)
data Predicate where
        Pred  :: String -> Predicate
        Pred1 :: forall a. (Show a, Storable a) => String -> a -> Predicate
        Pred2 :: Combinator -> Predicate -> Predicate -> Predicate
        Pred3 :: String -> Property -> Predicate
        Pred4 :: forall a. (Show a, Storable a) => String -> Property -> a -> Predicate

instance Show Predicate where
        show (Pred name)        = name
        show (Pred1 name a1)    = "(" ++ name ++ " " ++ show a1 ++ ")"
        show (Pred2 comb p1 p2) = "(" ++ show p1 ++ " " ++ show comb ++ " " ++ show p2 ++ ")"
        show (Pred3 name p)     = "(" ++ name ++ " " ++ show p ++ ")"
        show (Pred4 name p a)   = "(" ++ name ++ " " ++ show p ++ " " ++ show a ++ ")"

instance Serializable Predicate where
        serialize n (Pred name)   = ([FunDescr { functionalType  = PredicateFun,
                                                 functionalSymb  = name,
                                                 functionalArg   = Empty,
                                                 functionalLeft  = -1,
                                                 functionalRight = -1 }], n+1)

        serialize n (Pred1 name x) = ([FunDescr { functionalType  = PredicateFun,
                                                  functionalSymb  = name,
                                                  functionalArg   = ArgData $ StorableContext x,
                                                  functionalLeft  = -1,
                                                  functionalRight = -1 }], n+1)

        serialize n (Pred2 comb p1 p2) = let ([g'], n')   = serialize n comb
                                             (f'',  n'')  = serialize n' p1
                                             (f''', n''') = serialize n'' p2
                                         in ( [g'{ functionalLeft = n', functionalRight = n''}] ++ f'' ++ f''', n''')

        serialize n (Pred3 name p ) = let (f', n') = ([FunDescr { functionalType  = PredicateFun,
                                                 functionalSymb  = name,
                                                 functionalArg   = ArgFun (n+1),
                                                 functionalLeft  = -1,
                                                 functionalRight = -1 }], n+1)
                                          (f'', n'') = serialize n' p
                                      in (f' ++ f'', n'')

        serialize n (Pred4 name p x) = let (f', n') = ([FunDescr { functionalType  = PredicateFun,
                                                 functionalSymb  = name,
                                                 functionalArg   = ArgDataFun (StorableContext x) (n+1),
                                                 functionalLeft  = -1,
                                                 functionalRight = -1 }], n+1)
                                           (f'', n'') = serialize n' p
                                       in (f' ++ f'', n'')

-- NetFunction

data NetFunction f where
        Fun   :: String -> NetFunction f
        Fun1  :: forall a f. (Show a, Storable a) => String -> a -> NetFunction f
        HFun  :: String -> Predicate -> NetFunction f
        HFun1 :: String -> Predicate -> NetFunction f -> NetFunction f
        HFun2 :: String -> Predicate -> NetFunction f -> NetFunction f -> NetFunction f
        Comp  :: forall f1 f2 f. NetFunction f1 -> NetFunction f2 -> NetFunction f


instance Show (NetFunction f) where
        show (Fun name) = name
        show (Fun1 name a) = "(" ++ name ++ " " ++ show a ++ ")"
        show (HFun  name pred) = "(" ++ name ++ " " ++ show pred ++ ")"
        show (HFun1 name pred a) = "(" ++ name ++ " " ++ show pred ++ " (" ++ show a ++ "))"
        show (HFun2 name pred a1 a2)  = "(" ++ name ++ " " ++ show pred ++ " (" ++ show a1 ++ ") (" ++ show a2 ++ "))"
        show (Comp c1 c2) = show c1 ++ " >-> " ++ show c2


instance Serializable (NetFunction f) where
        serialize n (Fun name) = ([FunDescr { functionalType  = MonadicFun,
                                              functionalSymb  = name,
                                              functionalArg   = Empty,
                                              functionalLeft  = n+1,
                                              functionalRight = n+1 }], n+1)

        serialize n (Fun1 name x) = ([FunDescr { functionalType  = MonadicFun,
                                                 functionalSymb  = name,
                                                 functionalArg   = ArgData $ StorableContext x,
                                                 functionalLeft  = n+1,
                                                 functionalRight = n+1 }], n+1)

        serialize n (HFun name p) = let (s', n') = ([FunDescr { functionalType  = HighOrderFun,
                                                                functionalSymb  = name,
                                                                functionalArg   = ArgFun n',
                                                                functionalLeft  = n'',
                                                                functionalRight = n'' }], n+1)
                                        (p', n'') = serialize n' p
                                    in (s' ++ p', n'')

        serialize n (HFun1 name p c) = let (f', n') = (FunDescr { functionalType  = HighOrderFun,
                                                                  functionalSymb  = name,
                                                                  functionalArg   = ArgFun n',
                                                                  functionalLeft  = -1,
                                                                  functionalRight = -1 }, n+1)
                                           (p', n'') = serialize n' p
                                           (c', n''') = serialize n'' c
                                        in ( [f'{ functionalLeft = n''', functionalRight = n''}] ++ p' ++ c', n''')

        serialize n (HFun2 name p c1 c2) = let (f', n') = (FunDescr { functionalType  = HighOrderFun,
                                                                      functionalSymb  = name,
                                                                      functionalArg   = ArgFun n',
                                                                      functionalLeft  = -1,
                                                                      functionalRight = -1 }, n+1)
                                               (p',  n'') = serialize n' p
                                               (c1', n''') = serialize n'' c1
                                               (c2', n'''') = serialize n''' c2
                                            in ( [f'{ functionalLeft = n''', functionalRight = n''}] ++ p' ++ map (relinkFunDescr n''' n'''') c1' ++ c2', n'''')

        serialize n (Comp c1 c2) = let (s1, n') = serialize n c1
                                       (s2, n'') = serialize n' c2
                                   in (s1 ++ s2, n'')


-- InKernelFun: signature of basic in-kernel monadic function.

newtype SkBuff   = SkBuff ()
type Action      = Identity

-- operator: >->

(>->) :: NetFunction (a -> m b) -> NetFunction (b -> m c) -> NetFunction (a -> m c)
f1 >-> f2 = Comp f1 f2


